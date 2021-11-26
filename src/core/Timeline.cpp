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


#include <algorithm>
#include <core/Timeline.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

namespace H2Core
{

Timeline::Timeline() : Object( )
{
}

Timeline::~Timeline() {
	m_tempoMarkers.clear();
	m_tags.clear();
}

void Timeline::addTempoMarker( int nBar, float fBpm ) {
		
	if ( fBpm < MIN_BPM ) {
		fBpm = MIN_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
					.arg( fBpm ).arg( MIN_BPM ) );
	} else if ( fBpm > MAX_BPM ) {
		fBpm = MAX_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
					.arg( fBpm ).arg( MAX_BPM ) );
	}

	std::shared_ptr<TempoMarker> pTempoMarker = std::make_shared<TempoMarker>();
	pTempoMarker->nBar = nBar;
	pTempoMarker->fBpm = fBpm;

	m_tempoMarkers.push_back( pTempoMarker );
	sortTempoMarkers();
}

void Timeline::deleteTempoMarker( int nBar ) {

	// Erase the value to set the new value
	if ( m_tempoMarkers.size() >= 1 ){
		for ( int t = 0; t < m_tempoMarkers.size(); t++ ){
			if ( m_tempoMarkers[t]->nBar == nBar ) {
				m_tempoMarkers.erase( m_tempoMarkers.begin() +  t);
			}
		}
	}

	sortTempoMarkers();
}

float Timeline::getTempoAtBar( int nBar ) const {
	auto pHydrogen = Hydrogen::get_instance();
		
	if ( m_tempoMarkers.size() == 0 ) {
		return pHydrogen->getSong()->getBpm();
	}

	float fBpm;
	// When transport is stopped nBar is set to -1 by the
	// AudioEngine.
	if ( nBar == -1 ) {
		nBar = 0;
	}
	if ( isFirstTempoMarkerSpecial() && nBar < m_tempoMarkers[ 0 ]->nBar ) {
		fBpm = pHydrogen->getSong()->getBpm();
	} else {
		for ( int ii = 0; ii < static_cast<int>(m_tempoMarkers.size()); ii++) {
			if ( m_tempoMarkers[ ii ]->nBar > nBar ) {
				break;
			}
			fBpm = m_tempoMarkers[ ii ]->fBpm;
		}
	}
	return fBpm;
}

bool Timeline::isFirstTempoMarkerSpecial() const {
	if ( m_tempoMarkers.size() == 0 ) {
		return true;
	}

	return m_tempoMarkers[ 0 ]->nBar != 0;
}

const std::vector<std::shared_ptr<const Timeline::TempoMarker>> Timeline::getAllTempoMarkers() const {
	if ( isFirstTempoMarkerSpecial() ) {

		std::shared_ptr<TempoMarker> pTempoMarker = std::make_shared<TempoMarker>();
		pTempoMarker->nBar = 0;
		pTempoMarker->fBpm = Hydrogen::get_instance()->getSong()->getBpm();

		int nNumberOfTempoMarkers = m_tempoMarkers.size();
		std::vector<std::shared_ptr<const TempoMarker>> tmpVector;
		tmpVector.resize( nNumberOfTempoMarkers + 1 );
		tmpVector[ 0 ] = pTempoMarker;

		if ( nNumberOfTempoMarkers != 0 ) {
			for ( int ii = 0; ii < nNumberOfTempoMarkers; ++ii ) {
				// Since the returned vector is const, there is no
				// need to make a deep copy.
				tmpVector[ ii + 1 ] = m_tempoMarkers[ ii ];
			}
		}
		return tmpVector;
	}

	return m_tempoMarkers;
}
		
void Timeline::sortTempoMarkers() {
	sort( m_tempoMarkers.begin(), m_tempoMarkers.end(),
		  TempoMarkerComparator() );
}

void Timeline::addTag( int nBar, QString sTag ) {
		
	std::shared_ptr<Tag> pTag( new Tag );
	pTag->nBar = nBar;
	pTag->sTag = sTag;

	m_tags.push_back( std::move( pTag ) );
	sortTags();
}

void Timeline::deleteTag( int nBar ) {

	// Erase the value to set the new value
	if ( m_tags.size() >= 1 ){
		for ( int t = 0; t < m_tags.size(); t++ ){
			if ( m_tags[t]->nBar == nBar ) {
				m_tags.erase( m_tags.begin() +  t);
			}
		}
	}
		
	sortTags();
}

const QString Timeline::getTagAtBar( int nBar, bool bSticky ) const {

	QString sCurrentTag("");

	if ( bSticky ) {
		for ( int t = 0; t < static_cast<int>(m_tags.size()); t++ ){
			if ( m_tags[t]->nBar > nBar ){
				break;
			}
			sCurrentTag = m_tags[t]->sTag;
		}
	} else {
		for ( int t = 0; t < static_cast<int>(m_tags.size()); t++ ){
			if ( m_tags[t]->nBar == nBar ){
				sCurrentTag =  m_tags[t]->sTag;
			}
		}
	}

	return sCurrentTag;
}

void Timeline::sortTags()
{
	//sort the timeline vector to beats a < b
	sort(m_tags.begin(), m_tags.end(), TagComparator());
}

QString Timeline::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Timeline]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_tempoMarkers:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto const& tt : m_tempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( "%1[bar: %2 , bpm: %3]\n" ).arg( sPrefix + s + s ).arg( tt->nBar ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "%1%2m_tags:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto const& tt : m_tags ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( "%1[bar: %2 , tag: %3]\n" ).arg( sPrefix + s + s ).arg( tt->nBar ).arg( tt->sTag ) );
			}
		}
	} else {
		
		sOutput = QString( "%1[Timeline] " ).arg( sPrefix )
			.append( QString( "m_tempoMarkers: [" ) );
		for ( auto const& tt : m_tempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( " [bar: %1 , bpm: %2]" ).arg( tt->nBar ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "], m_tags: [" ) );
		for ( auto const& tt : m_tags ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( " [bar: %1 , tag: %2]" ).arg( tt->nBar ).arg( tt->sTag ) );
			}
		}
		sOutput.append(" ]");
	}
		
	return sOutput;
}

};

