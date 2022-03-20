/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2_JACK_OUTPUT_H
#define H2_JACK_OUTPUT_H

#include <core/IO/AudioOutput.h>
#include <core/IO/NullDriver.h>

// check if jack support is disabled
#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_
// JACK support is enabled.

#include <map>
#include <memory>
#include <pthread.h>
#include <jack/jack.h>
#include <jack/transport.h>

#include <core/Globals.h>



namespace H2Core
{
	
class Song;
class Instrument;
class InstrumentComponent;

/**
 * JACK (Jack Audio Connection Kit) server driver.
 *
 * __Transport Control__:
 *
 * Each JACK client can start and stop the transport or relocate the
 * current transport position. The request will take place in
 * cycles. During the first the status of the transport changes to
 * _JackTransportStarting_ to inform all clients a change is about to
 * happen. During the second the status is again
 * _JackTransportRolling_ and the transport position is updated
 * according to the request.
 *
 * Also note that Hydrogen overwrites its local TransportInfo only
 * with the transport position of the JACK server if there is a
 * mismatch due to a relocation triggered by another JACK
 * client. During normal transport the current position
 * TransportInfo::m_nFrames will be always the same as the one of JACK
 * during a cycle and incremented by the buffer size in
 * audioEngine_process() at the very end of the cycle. The same
 * happens for the transport information of the JACK server but in
 * parallel.
 *
 * __Timebase Master__:
 *
 * The timebase master is responsible for providing additional
 * transport information to the JACK server apart from the transport
 * position in frames, like current beat, bar, tick, tick size, speed
 * etc. Of all these information Hydrogen does only use the provided
 * tempo (and overrides all internal ones). Therefore, unlike many
 * other application, it does _not_ respond to changes in measure
 * (since these would have to be mapped to the lenght of the current
 * pattern). Every client can be registered as timebase master by
 * supplying a callback (for Hydrogen this would be
 * JackTimebaseCallback()) but there can be at most one timebase
 * master at a time. Having none at all is perfectly fine too. Apart
 * from this additional responsibility, the registered client has no
 * other rights compared to others.
 *
 * After the status of the JACK transport has changed from
 * _JackTransportStarting_ to _JackTransportRolling_, the timebase
 * master needs an additional cycle to update its information.
 */
/** \ingroup docCore docAudioDriver */
class JackAudioDriver : public Object<JackAudioDriver>, public AudioOutput
{
	H2_OBJECT(JackAudioDriver)
public:
	/**
	 * Whether Hydrogen or another program is Jack timebase master.
	 */
	enum class Timebase {
		/** Hydrogen itself is timebase master.*/
		Master = 1,
		/** An external program is timebase master and Hydrogen will
         * disregard all tempo markers on the Timeline and, instead,
         * only use the BPM provided by JACK.*/
		Slave = 0,
		/** Only normal clients registered */
		None = -1
	};
	
	/** 
	 * Object holding the external client session with the JACK
	 * server.
	 */
	jack_client_t* m_pClient;
	/**
	 * Constructor of the JACK server driver.
	 *
	 * @param m_processCallback Function that is called by the
			 AudioEngine during every processing cycle.
	 */
	JackAudioDriver( JackProcessCallback m_processCallback );
	/**
	 * Destructor of the JACK server driver.
	 *
	 * Calling disconnect().
	 */
	~JackAudioDriver();

	/**
	 * Connects to output ports via the JACK server.
	 *	 *
	 * @return 
	 * - __0__ : on success.
	 * - __1__ : The activation of the JACK client using
	 *           did fail.
	 * - __2__ : The connections to #m_sOutputPortName1 and
	 *       #m_sOutputPortName2 could not be established and the
	 *       there were either no JACK ports holding the
	 *       JackPortIsInput flag found or no connection to them could
	 *       be established.
	 */
	virtual int connect() override;
	/**
	 * Disconnects the JACK client of the Hydrogen from the JACK
	 * server.
	 */
	virtual void disconnect() override;
	/**
	 * Deactivates the JACK client of Hydrogen and disconnects all
	 * ports belonging to it.
	 */
	void deactivate();
	/** \return Global variable #jackServerBufferSize. */
	virtual unsigned getBufferSize() override;
	/** \return Global variable #jackServerSampleRate. */
	virtual unsigned getSampleRate() override;

	virtual int getXRuns() const override;

	/** Resets the buffers contained in #m_pTrackOutputPortsL and
	 * #m_pTrackOutputPortsR.
	 * 
	 * @param nFrames Size of the buffers used in the audio process
	 * callback function.
	 */
	void clearPerTrackAudioBuffers( uint32_t nFrames );
	
	/**
	 * Creates per component output ports for each instrument.
	 */
	void makeTrackOutputs( std::shared_ptr<Song> pSong );

	/** \param flag Sets #m_bConnectDefaults*/
	void setConnectDefaults( bool flag ) {
		m_bConnectDefaults = flag;
	}
	/** \return #m_bConnectDefaults */
	bool getConnectDefaults() {
		return m_bConnectDefaults;
	}

	/**
	 * Get content in the left stereo output port.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	virtual float* getOut_L() override;
	/**
	 * Get content in the right stereo output port.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	virtual float* getOut_R() override;
	/**
	 * Get content of left output port of a specific track.
	 *
	 * \param nTrack Track number. Must not be bigger than
	 * #m_nTrackPortCount.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( unsigned nTrack );
	/**
	 * Get content of right output port of a specific track.
	 *
	 * \param nTrack Track number. Must not be bigger than
	 * #m_nTrackPortCount.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( unsigned nTrack );
	/** 
	 * Convenience function looking up the track number of a component
	 * of an instrument using in #m_trackMap using their IDs
	 * Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using the
	 * track number it then calls getTrackOut_L( unsigned ) and
	 * returns its result.
	 *
	 * \param instr Pointer to an Instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( std::shared_ptr<Instrument> instr, std::shared_ptr<InstrumentComponent> pCompo );
	/** 
	 * Convenience function looking up the track number of a component
	 * of an instrument using in #m_trackMap using their IDs
	 * Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using the
	 * track number it then calls getTrackOut_R( unsigned ) and
	 * returns its result.
	 *
	 * \param instr Pointer to an Instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( std::shared_ptr<Instrument> instr, std::shared_ptr<InstrumentComponent> pCompo );

	/**
	 * Initializes the JACK audio driver.
	 *
	 * \param bufferSize Unused and only present to assure
	 * compatibility with the JACK API.
	 *
	 * \return
	 * -  __0__ : on success.
	 * - __-1__ : if the pointer #m_pClient obtained via
	 * _jack_client_open()_ (jack/jack.h) is 0.
	 * - __4__ : unable to register the "out_L" and/or "out_R"
	 * output ports for the JACK client using
	 * _jack_port_register()_ (jack/jack.h).
	 */
	virtual int init( unsigned bufferSize ) override;

	/**
	 * Tells the JACK server to start transport.
	 */
	void startTransport();
	/**
	 * Tells the JACK server to stop transport.
	 */
	void stopTransport();
	/**
	 * Re-positions the transport position to @a nFrame.
	 *
	 * The new position takes effect in two process cycles during
	 * which JACK's state will be in JackTransportStarting and the
	 * transport won't be rolling.
	 *   
	 * \param nFrame Requested new transport position.
	 */
	void locateTransport( long long nFrame );
	/**
	 * The function queries the transport position and additional
	 * information from the JACK server, writes them to
	 * #m_JackTransportPos and in #m_JackTransportState, and updates
	 * the AudioEngine in case of a mismatch.
	 */
	void updateTransportInfo();

	/**
	 * Registers Hydrogen as JACK timebase master.
	 */ 
	void initTimebaseMaster();
	/**
	 * Release Hydrogen from the JACK timebase master
	 *  responsibilities.
	 *
	 * This causes the JackTimebaseCallback() callback function to not
	 * be called by the JACK server anymore.
	 */
	void releaseTimebaseMaster();
	
	/**
	 * \return #m_timebaseState
	 */
	Timebase getTimebaseState() const;
	
	/**
	 * Sample rate of the JACK audio server.
	 */
	static unsigned long		jackServerSampleRate;
	/**
	 * Buffer size of the JACK audio server.
	 */
	static jack_nframes_t		jackServerBufferSize;
	/**
	 * Instance of the JackAudioDriver.
	 */
	static JackAudioDriver*		pJackDriverInstance;
	/** Number of XRuns since the driver started.*/
	static int jackServerXRuns;

	/**
	 * Callback function for the JACK audio server to set the sample
	 * rate #jackServerSampleRate.
	 *
	 * \param nframes New sample rate.
	 * \param param Object inheriting from the #Logger class.
	 *
	 * @return 0 on success
	 */
	static int jackDriverSampleRate( jack_nframes_t nframes, void* param );
	
	/**
	 * Callback function for the JACK audio server to set the buffer
	 * size #jackServerBufferSize.
	 *
	 * \param nframes New buffer size.
	 * \param arg Not used
	 *
	 * @return 0 on success
	 */
	static int jackDriverBufferSize( jack_nframes_t nframes, void* arg );
	/** Report an XRun event to the GUI.*/
	static int jackXRunCallback( void* arg );

	/** \return the BPM reported by the timebase master or NAN if there
		is no external timebase master.*/
	float getMasterBpm() const;

	/** 
	 * Uses the bar-beat-tick information to relocate the transport
	 * position.
	 *
	 * This type of operation is triggered whenever the transport
	 * position gets relocated or the tempo is changed using Jack in
	 * the presence of an external timebase master.*/
	void relocateUsingBBT();

private:

	/** Compares the BBT information stored in #m_JackTransportPos and
	 * #m_previousJackTransportPos with respect to the tempo and the
	 * transport position in bars, beats, and ticks.
	 *
	 * @return true If #m_JackTransportPos is expected to follow
	 * #m_previousJackTransportPos.
	 */
	bool compareAdjacentBBT() const;
	
	/**
	 * Callback function for the JACK server to supply additional
	 * timebase information.
	 *
	 * The function it will be called after the
	 * audioEngine_process() function and only if the
	 * #m_JackTransportState is _JackTransportRolling_.
	 *
	 * What is the BBT information?
	 *
	 * There is no formal definition in the JACK API but the way it is
	 * interpreted by Hydrogen is the following:
	 *
	 * bar: Number of patterns passed since the beginning of the
	 * song.
	 * beat: Number of quarters passed since the beginning of the
	 *     pattern. 
	 * tick: Number of ticks passed since the last beat (with respect
	 *     to the current frame). 
	 *
	 * A tick is an internal measure representing the smallest
	 * resolution of the transport position in terms of the
	 * patterns. It consist of pAudioEngine->getTickSize() frames, which
	 * changes depending on the current tempo.
	 *
	 * \param state Unused.
	 * \param nFrames Unused.
	 * \param pJackPosition Current transport position.
	 * \param new_pos Unused.
	 * \param arg Pointer to a JackAudioDriver instance.
	 */
	static void JackTimebaseCallback( jack_transport_state_t state,
					    jack_nframes_t nFrames,
					    jack_position_t* pJackPosition,
					    int new_pos,
					    void* arg );
	/**
	 * Callback function for the JACK audio server to shutting down the
	 * JACK driver.
	 *
	 * \param arg Unused.
	 */	
	static void jackDriverShutdown( void* arg );

	static void printJackTransportPos( const jack_position_t* pPos );

	/** Show debugging information.*/
	void printState() const;

	/**
	 * Renames the @a n 'th port of JACK client and creates it if
	 * it's not already present. 
	 *
	 * If the track number @a n is bigger than the number of ports
	 * currently in usage #m_nTrackPortCount, @a n + 1 -
	 * #m_nTrackPortCount new stereo ports will be created using
	 * _jack_port_register()_ (jack/jack.h) and #m_nTrackPortCount
	 * updated to @a n + 1.
	 *
	 * Afterwards, the @a n 'th port is renamed to a concatenation of
	 * "Track_", DrumkitComponent::__name, "_", @a n + 1, "_",
	 * Instrument::__name, and "_L", or "_R" using either
	 * _jack_port_rename()_ (if HAVE_JACK_PORT_RENAME is defined) or
	 * _jack_port_set_name()_ (both jack/jack.h). The former renaming
	 * function triggers a _PortRename_ notifications to clients that
	 * have registered a port rename handler.
	 *
	 * \param n Track number for which a port should be renamed
	 *   (and created).
	 * \param instr Pointer to the corresponding Instrument.
	 * \param pCompo Pointer to the corresponding
	 *   InstrumentComponent.
	 * \param pSong Pointer to the corresponding Song.
	 */
	void setTrackOutput( int n, std::shared_ptr<Instrument> instr, std::shared_ptr<InstrumentComponent> pCompo, std::shared_ptr<Song> pSong );
	/** Main process callback. */
	JackProcessCallback		m_processCallback;
	/**
	 * Left source port.
	 */
	jack_port_t*			m_pOutputPort1;
	/**
	 * Right source port.
	 */
	jack_port_t*			m_pOutputPort2;
	/**
	 * Destination of the left source port #m_pOutputPort1, for which
	 * a connection will be established in connect().
	 */
	QString				m_sOutputPortName1;
	/**
	 * Destination of the right source port #m_pOutputPort2, for which
	 * a connection will be established in connect().
	 */
	QString				m_sOutputPortName2;
	/**
	 * Matrix containing the track number of each component of all
	 * instruments. Its rows represent the instruments and its columns
	 * their components. _m_trackMap[2][1]=6_ thus therefore mean the
	 * output of the second component of the third instrument is
	 * assigned the seventh output port. Since its total size is
	 * defined by #MAX_INSTRUMENTS and #MAX_COMPONENTS, most of its
	 * entries will be zero.
	 */
	int				m_trackMap[MAX_INSTRUMENTS][MAX_COMPONENTS];
	/**
	 * Total number of output ports currently in use.
	 */
	int				m_nTrackPortCount;
	/**
	 * Vector of all left audio output ports currently used by the
	 * local JACK client.
	 */
	jack_port_t*			m_pTrackOutputPortsL[MAX_INSTRUMENTS];
	/**
	 * Vector of all right audio output ports currently used by the
	 * local JACK client.
	 */
	jack_port_t*		 	m_pTrackOutputPortsR[MAX_INSTRUMENTS];

	/**
	 * Current transport state returned by
	 * _jack_transport_query()_ (jack/transport.h).  
	 *
	 * It is valid for the entire cycle and can have five
	 * different values:
	 * - _JackTransportStopped_ = 0 : Transport is halted
	 * - _JackTransportRolling_ = 1 : Transport is playing
	 * - _JackTransportLooping_ = 2 : For OLD_TRANSPORT, now
	 *   ignored
	 * - _JackTransportStarting_ = 3 : Waiting for sync ready 
	 * - _JackTransportNetStarting_ = 4 : Waiting for sync ready
	 *   on the network
	 *
	 * The actual number of states depends on your JACK
	 * version. The listing above was done for version 1.9.12.
	 */
	jack_transport_state_t		m_JackTransportState;
	/**
	 * Current transport position obtained using
	 * _jack_transport_query()_ (jack/transport.h).
	 *
	 * It corresponds to the first frame of the current cycle.
	 *
	 * The __valid__ member of #m_JackTransportPos will show which
	 * fields contain valid data. Thus, if it is set to
	 * _JackPositionBBT_, bar, beat, and tick information are
	 * provided by the current timebase master in addition to the
	 * transport information in frames. It is of class
	 * _jack_position_bits_t_ (jack/types.h) and is an enumerator
	 * with five different options:
	 * - _JackPositionBBT_ = 0x10 : Bar, Beat, Tick 
	 * - _JackPositionTimecode_ = 0x20 : External timecode 
	 * - _JackBBTFrameOffset_ = 0x40 : Frame offset of BBT
	 *   information
	 * - _JackAudioVideoRatio_ = 0x80 : audio frames per video
	 *   frame
	 * - _JackVideoFrameOffset_ = 0x100 : frame offset of first
	 *   video frame
	 */
	jack_position_t			m_JackTransportPos;
	/** Used for detecting changes in the BBT transport information
	 * with external timebase master application, which do not
	 * propagate these changes on time.
	 */
	jack_position_t			m_previousJackTransportPos;

	/**
	 * Specifies whether the default left and right (master) audio
	 * JACK ports will be automatically connected to the system's sink
	 * when registering the JACK client in connect().
	 */
	bool				m_bConnectDefaults;
	/**
	 * Whether Hydrogen or another program is Jack timebase master.
	 *
	 * - #m_nTimebaseTracking > 0 - Hydrogen itself is timebase
          master.
	 * - #m_nTimebaseTracking == 0 - an external program is timebase
          master and Hydrogen will disregard all tempo marker on the
          Timeline and, instead, only use the BPM provided by JACK.
	 * - #m_nTimebaseTracking < 0 - only normal clients registered.
	 *
	 * While Hydrogen can unregister as timebase master on its own, it
	 * can not be observed directly whether another application has
	 * taken over as timebase master. When the JACK server is
	 * releasing Hydrogen in the later case, it won't advertise this
	 * fact but simply won't call the JackTimebaseCallback()
	 * anymore. But since this will be called in every cycle after
	 * updateTransportInfo(), we can use this variable to determine if
	 * Hydrogen is still timebase master.
	 *
	 * As Hydrogen registered as timebase master using
	 * initTimebaseMaster() it will be initialized with 1, decremented
	 * in updateTransportInfo(), and reset to 1 in
	 * JackTimebaseCallback(). Whenever it is zero in
	 * updateTransportInfo(), #m_nTimebaseTracking will be updated
	 * accordingly.
	 */
	int				m_nTimebaseTracking;

	/**
	 * More user-friendly version of #m_nTimebaseTracking.
	 */ 
	Timebase m_timebaseState;

};

}; // H2Core namespace


#else // H2CORE_HAVE_JACK
// JACK is disabled

namespace H2Core {
/** \ingroup docCore docAudioDriver */
class JackAudioDriver : public NullDriver {
	H2_OBJECT(JackAudioDriver)
public:
	/**
	 * Whether Hydrogen or another program is Jack timebase master.
	 */
	enum class Timebase {
		/** Hydrogen itself is timebase master.*/
		Master = 1,
		/** An external program is timebase master and Hydrogen will
         * disregard all tempo marker on the Timeline and, instead,
         * only use the BPM provided by JACK.*/
		Slave = 0,
		/** Only normal clients registered */
		None = -1
	};
	/**
	 * Fallback version of the JackAudioDriver in case
	 * #H2CORE_HAVE_JACK was not defined during the configuration
	 * and the usage of the JACK audio server is not intended by
	 * the user.
	 */
	JackAudioDriver( audioProcessCallback m_processCallback ) : NullDriver( m_processCallback ) {}

	// Required since these functions are a friend of AudioEngine which
	// need to be build even if no JACK support is desired.
	void updateTransportInfo() {}
	void relocateUsingBBT() {}
};

}; // H2Core namespace


#endif // H2CORE_HAVE_JACK

#endif

