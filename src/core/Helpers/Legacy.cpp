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

#include <memory>

#include <core/Helpers/Legacy.h>

#include "Version.h"
#include <core/Helpers/Xml.h>
#include <core/Basics/Song.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Note.h>
#include <core/Basics/Adsr.h>

namespace H2Core {


Drumkit* Legacy::load_drumkit( const QString& dk_path, bool bSilent ) {
	WARNINGLOG( QString( "loading drumkit with legacy code" ) );
	
	XMLDoc doc;
	if( !doc.read( dk_path, nullptr, bSilent ) ) {
		return nullptr;
	}
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return nullptr;
	}
	QString drumkit_name = root.read_string( "name", "", false, false, bSilent );
	if ( drumkit_name.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return nullptr;
	}
	Drumkit* pDrumkit = new Drumkit();
	pDrumkit->set_path( dk_path.left( dk_path.lastIndexOf( "/" ) ) );
	pDrumkit->set_name( drumkit_name );
	pDrumkit->set_author( root.read_string( "author", "undefined author",
											false, false, bSilent ) );
	pDrumkit->set_info( root.read_string( "info", "defaultInfo",
											false, false, bSilent ) );
	pDrumkit->set_license( root.read_string( "license", "undefined license",
											false, false, bSilent ) );
	pDrumkit->set_image( root.read_string( "image", "",
											false, false, bSilent ) );
	pDrumkit->set_image_license( root.read_string( "imageLicense", "undefined license",
											false, false, bSilent ) );

	XMLNode instruments_node = root.firstChildElement( "instrumentList" );
	if ( instruments_node.isNull() ) {
		if ( ! bSilent ) {
			WARNINGLOG( "instrumentList node not found" );
		}
		pDrumkit->set_instruments( new InstrumentList() );
	} else {
		InstrumentList* pInstruments = new InstrumentList();
		XMLNode instrument_node = instruments_node.firstChildElement( "instrument" );
		int count = 0;
		while ( !instrument_node.isNull() ) {
			count++;
			if ( count > MAX_INSTRUMENTS ) {
				ERRORLOG( QString( "instrument count >= %2, stop reading instruments" )
						  .arg( MAX_INSTRUMENTS ) );
				break;
			}
			std::shared_ptr<Instrument> pInstrument = nullptr;
			int id = instrument_node.read_int( "id", EMPTY_INSTR_ID, false, false );
			if ( id!=EMPTY_INSTR_ID ) {
				pInstrument =
					std::make_shared<Instrument>( id, instrument_node.read_string( "name", "",
																				   false, false,
																				   bSilent  ),
												  nullptr );
				pInstrument->set_drumkit_name( drumkit_name );
				pInstrument->set_volume( instrument_node.read_float( "volume", 1.0f,
																	 true, true, bSilent ) );
				pInstrument->set_muted( instrument_node.read_bool( "isMuted", false,
																   true, true, bSilent ) );
				float fPanL = instrument_node.read_float( "pan_L", 1.f,
														  true, true, bSilent );
				float fPanR = instrument_node.read_float( "pan_R", 1.f,
														  true, true, bSilent );
				float fPan = Sampler::getRatioPan( fPanL, fPanR ); // convert to single pan parameter				
				pInstrument->setPan( fPan );

				// may not exist, but can't be empty
				pInstrument->set_apply_velocity( instrument_node.read_bool( "applyVelocity", true,
																			false, true, bSilent ) );
				pInstrument->set_filter_active( instrument_node.read_bool( "filterActive", true,
																		   false, true, bSilent ) );
				pInstrument->set_filter_cutoff( instrument_node.read_float( "filterCutoff", 1.0f,
																			true, false, bSilent ) );
				pInstrument->set_filter_resonance( instrument_node.read_float( "filterResonance", 0.0f,
																			   true, false, bSilent ) );
				pInstrument->set_random_pitch_factor( instrument_node.read_float( "randomPitchFactor", 0.0f,
																				  true, false, bSilent ) );
				float attack = instrument_node.read_float( "Attack", 0.0f,
														   true, false, bSilent );
				float decay = instrument_node.read_float( "Decay", 0.0f,
														  true, false, bSilent  );
				float sustain = instrument_node.read_float( "Sustain", 1.0f,
															true, false, bSilent );
				float release = instrument_node.read_float( "Release", 1000.0f,
															true, false, bSilent );
				pInstrument->set_adsr( std::make_shared<ADSR>( attack, decay, sustain, release ) );
				pInstrument->set_gain( instrument_node.read_float( "gain", 1.0f,
																   true, false, bSilent ) );
				pInstrument->set_mute_group( instrument_node.read_int( "muteGroup", -1,
																	   true, false, bSilent ) );
				pInstrument->set_midi_out_channel( instrument_node.read_int( "midiOutChannel", -1,
																			 true, false, bSilent ) );
				pInstrument->set_midi_out_note( instrument_node.read_int( "midiOutNote", MIDI_MIDDLE_C,
																		  true, false, bSilent ) );
				pInstrument->set_stop_notes( instrument_node.read_bool( "isStopNote", true,
																		false, true, bSilent ) );
				QString read_sample_select_algo = instrument_node.read_string( "sampleSelectionAlgo", "VELOCITY",
																			   true, true, bSilent );
				
				if ( read_sample_select_algo.compare("VELOCITY") == 0) {
					pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
				} else if ( read_sample_select_algo.compare("ROUND_ROBIN") == 0 ) {
					pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
				} else if ( read_sample_select_algo.compare("RANDOM") == 0 ) {
					pInstrument->set_sample_selection_alg( Instrument::RANDOM );
				}
				
				pInstrument->set_hihat_grp( instrument_node.read_int( "isHihat", -1,
																	  true, true, bSilent ) );
				pInstrument->set_lower_cc( instrument_node.read_int( "lower_cc", 0,
																	 true, true, bSilent ) );
				pInstrument->set_higher_cc( instrument_node.read_int( "higher_cc", 127,
																	  true, true, bSilent ) );
				for ( int i=0; i<MAX_FX; i++ ) {
					pInstrument->set_fx_level( instrument_node.read_float( QString( "FX%1Level" ).arg( i+1 ), 0.0,
																		   true, true, bSilent ), i );
				}
				QDomNode filename_node = instrument_node.firstChildElement( "filename" );
				if ( !filename_node.isNull() ) {
					if ( ! bSilent ) {
						DEBUGLOG( "Using back compatibility code. filename node found" );
					}
					QString sFilename = instrument_node.read_string( "filename", "",
																	 true, true, bSilent);
					if( sFilename.isEmpty() ) {
						ERRORLOG( "filename back compatibility node is empty" );
					} else {

						auto pSample = std::make_shared<Sample>( dk_path+"/"+sFilename );

						bool bFoundMainCompo = false;
						for (std::vector<DrumkitComponent*>::iterator it = pDrumkit->get_components()->begin() ; it != pDrumkit->get_components()->end(); ++it) {
							DrumkitComponent* pExistingComponent = *it;
							if( pExistingComponent->get_name().compare("Main") == 0) {
								bFoundMainCompo = true;
								break;
							}
						}
						
						if ( !bFoundMainCompo ) {
							DrumkitComponent* pDrumkitCompo = new DrumkitComponent( 0, "Main" );
							pDrumkit->get_components()->push_back( pDrumkitCompo );
						}
						
						auto pComponent = std::make_shared<InstrumentComponent>( 0 );
						auto pLayer = std::make_shared<InstrumentLayer>( pSample );
						pComponent->set_layer( pLayer, 0 );
						pInstrument->get_components()->push_back( pComponent );
						
					}
				} else {
					int n = 0;
					bool bFoundMainCompo = false;
					for (std::vector<DrumkitComponent*>::iterator it = pDrumkit->get_components()->begin() ; it != pDrumkit->get_components()->end(); ++it) {
						DrumkitComponent* pExistingComponent = *it;
						if( pExistingComponent->get_name().compare("Main") == 0) {
							bFoundMainCompo = true;
							break;
						}
					}
					
					if ( !bFoundMainCompo ) {
						DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
						pDrumkit->get_components()->push_back(pDrumkitComponent);
					}
					auto pComponent = std::make_shared<InstrumentComponent>( 0 );

					XMLNode layer_node = instrument_node.firstChildElement( "layer" );
					while ( !layer_node.isNull() ) {
						if ( n >= InstrumentComponent::getMaxLayers() ) {
							ERRORLOG( QString( "n (%1) > m_nMaxLayers (%2)" ).arg ( n ).arg( InstrumentComponent::getMaxLayers() ) );
							break;
						}
						auto pSample = std::make_shared<Sample>( dk_path + "/" +
																 layer_node.read_string( "filename", "",
																						 true, true, bSilent ) );
						auto pLayer = std::make_shared<InstrumentLayer>( pSample );
						pLayer->set_start_velocity( layer_node.read_float( "min", 0.0,
																		   true, true, bSilent ) );
						pLayer->set_end_velocity( layer_node.read_float( "max", 1.0,
																		 true, true, bSilent ) );
						pLayer->set_gain( layer_node.read_float( "gain", 1.0,
																 true, false, bSilent ) );
						pLayer->set_pitch( layer_node.read_float( "pitch", 0.0,
																  true, false, bSilent ) );
						pComponent->set_layer( pLayer, n );
						n++;
						layer_node = layer_node.nextSiblingElement( "layer" );
					}
					pInstrument->get_components()->push_back( pComponent );
				}
			}
			if( pInstrument ) {
				( *pInstruments ) << pInstrument;
			} else {
				ERRORLOG( QString( "Empty ID for instrument %1. The drumkit is corrupted. Skipping instrument" ).arg( count ) );
				count--;
			}
			instrument_node = instrument_node.nextSiblingElement( "instrument" );
		}
		pDrumkit->set_instruments( pInstruments );
	}
	return pDrumkit;
}

Pattern* Legacy::load_drumkit_pattern( const QString& pattern_path, InstrumentList* instrList ) {
	Pattern* pPattern = nullptr;
	if ( version_older_than( 0, 9, 8 ) ) {
		WARNINGLOG( QString( "this code should not be used anymore, it belongs to 0.9.6" ) );
	} else {
		WARNINGLOG( QString( "loading pattern with legacy code" ) );
	}
	XMLDoc doc;
	if( !doc.read( pattern_path ) ) {
		return nullptr;
	}
	XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_pattern node not found" );
		return nullptr;
	}
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	if ( pattern_node.isNull() ) {
		WARNINGLOG( "pattern node not found" );
		return nullptr;
	} else {
		QString sName = pattern_node.read_string( "pattern_name", "" );
		QString sInfo = pattern_node.read_string( "info", "" );
		QString sCategory = pattern_node.read_string( "category", "" );
		int nSize = pattern_node.read_int( "size", -1, false, false );
		
        //default nDenominator = 4 since old patterns have not <denominator> setting
		pPattern = new Pattern( sName, sInfo, sCategory, nSize, 4 );

		XMLNode note_list_node = pattern_node.firstChildElement( "noteList" );

		XMLNode note_node = note_list_node.firstChildElement( "note" );
		while ( !note_node.isNull() ) {
			Note* pNote = nullptr;
			unsigned nPosition = note_node.read_int( "position", 0 );
			float fLeadLag = note_node.read_float( "leadlag", 0.0 , false , false);
			float fVelocity = note_node.read_float( "velocity", 0.8f );
			float fPanL = note_node.read_float( "pan_L", 0.5 );
			float fPanR = note_node.read_float( "pan_R", 0.5 );
			float fPan = Sampler::getRatioPan( fPanL, fPanR ); // convert to single pan parameter

			int nLength = note_node.read_int( "length", -1, true );
			float nPitch = note_node.read_float( "pitch", 0.0, false, false );
			float fProbability = note_node.read_float( "probability", 1.0 , false , false );
			QString sKey = note_node.read_string( "key", "C0", false, false );
			QString nNoteOff = note_node.read_string( "note_off", "false", false, false );
			int instrId = note_node.read_int( "instrument", 0, true );

			auto instrRef = instrList->find( instrId );
			if ( !instrRef ) {
				ERRORLOG( QString( "Instrument with ID: '%1' not found. Note skipped." ).arg( instrId ) );
				note_node = note_node.nextSiblingElement( "note" );
				
				continue;
			}
			//assert( instrRef );
			bool noteoff = false;
			if ( nNoteOff == "true" ) {
				noteoff = true;
			}

			pNote = new Note( instrRef, nPosition, fVelocity, fPan, nLength, nPitch);
			pNote->set_key_octave( sKey );
			pNote->set_lead_lag(fLeadLag);
			pNote->set_note_off( noteoff );
			pNote->set_probability( fProbability );
			pPattern->insert_note( pNote );

			note_node = note_node.nextSiblingElement( "note" );
		}
	}
	return pPattern;
}

Playlist* Legacy::load_playlist( Playlist* pPlaylist, const QString& pl_path )
{
	if ( version_older_than( 0, 9, 8 ) ) {
		WARNINGLOG( QString( "this code should not be used anymore, it belongs to 0.9.6" ) );
	} else {
		WARNINGLOG( QString( "loading playlist with legacy code" ) );
	}
	XMLDoc doc;
	if( !doc.read( pl_path ) ) {
		return nullptr;
	}
	XMLNode root = doc.firstChildElement( "playlist" );
	if ( root.isNull() ) {
		ERRORLOG( "playlist node not found" );
		return nullptr;
	}
	QFileInfo fileInfo = QFileInfo( pl_path );
	QString filename = root.read_string( "Name", "", false, false );
	if ( filename.isEmpty() ) {
		ERRORLOG( "Playlist has no name, abort" );
		return nullptr;
	}

	pPlaylist->setFilename( pl_path );

	XMLNode songsNode = root.firstChildElement( "Songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "next" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "song", "", false, false );
			if ( !songPath.isEmpty() ) {
				Playlist::Entry* entry = new Playlist::Entry();
				QFileInfo songPathInfo( fileInfo.absoluteDir(), songPath );
				entry->filePath = songPathInfo.absoluteFilePath();
				entry->fileExists = songPathInfo.isReadable();
				entry->scriptPath = nextNode.read_string( "script", "" );
				entry->scriptEnabled = nextNode.read_bool( "enabled", false );
				pPlaylist->add( entry );
			}

			nextNode = nextNode.nextSiblingElement( "next" );
		}
	} else {
		WARNINGLOG( "Songs node not found" );
	}
	return pPlaylist;
}

};

/* vim: set softtabstop=4 noexpandtab: */
