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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>


using namespace H2Core;

#include <QTimer>
#include <QPainter>


#include "PatternEditorRuler.h"
#include "PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../Skin.h"


PatternEditorRuler::PatternEditorRuler( QWidget* parent )
 : QWidget( parent )
 {
	setAttribute(Qt::WA_OpaquePaintEvent);

	//infoLog( "INIT" );

	Preferences *pPref = Preferences::get_instance();

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	m_pPattern = nullptr;
	m_fGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();

	m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nRulerHeight = 25;
	
	m_nTicks = 0;

	resize( m_nRulerWidth, m_nRulerHeight );

	bool ok = m_tickPosition.load( Skin::getImagePath() + "/patternEditor/tickPosition.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap " );
	}

	m_pBackground = nullptr;
	createBackground();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateEditor()));

	HydrogenApp::get_instance()->addEventListener( this );
}



PatternEditorRuler::~PatternEditorRuler() {
	//infoLog( "DESTROY");
}



void PatternEditorRuler::updateStart(bool start) {
	if (start) {
		m_pTimer->start(50);	// update ruler at 20 fps
	}
	else {
		m_pTimer->stop();
	}
}



void PatternEditorRuler::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateEditor();
	updateStart(true);
}



void PatternEditorRuler::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
	updateStart(false);
}



void PatternEditorRuler::updateEditor( bool bRedrawAll )
{
	static int oldNTicks = 0;

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if( pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() )  ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}


	bool bActive = false;	// is the pattern playing now?

	/* 
	 * Lock audio engine to make sure pattern list does not get
	 * modified / cleared during iteration 
	 */
	pAudioEngine->lock( RIGHT_HERE );

	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();
	for ( int ii = 0; ii < pPlayingPatterns->size(); ++ii ) {
		auto ppattern = pPlayingPatterns->get( ii );
		if ( m_pPattern == ppattern ) {
			bActive = true;
			break;
		}
	}

	pAudioEngine->unlock();

	if ( ( pAudioEngine->getState() == H2Core::AudioEngine::State::Playing ) && bActive ) {
		m_nTicks = pAudioEngine->getPatternTickPosition();
	}
	else {
		m_nTicks = -1;	// hide the tickPosition
	}


	if (oldNTicks != m_nTicks) {
		// redraw all
		bRedrawAll = true;
	}
	oldNTicks = m_nTicks;

	if (bRedrawAll) {
		update( 0, 0, width(), height() );
	}
}


void PatternEditorRuler::createBackground()
{
	auto pPref = H2Core::Preferences::get_instance();

	if ( m_pBackground ) {
		delete m_pBackground;
	}

	// Create new background pixmap at native device pixelratio
	qreal pixelRatio = devicePixelRatio();
	m_pBackground = new QPixmap( pixelRatio * QSize( m_nRulerWidth, m_nRulerHeight ) );
	m_pBackground->setDevicePixelRatio( pixelRatio );

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
	m_pBackground->fill( backgroundColor );

	QPainter painter( m_pBackground );

	// gray background for unusable section of pattern
	if (m_pPattern) {
		int nXStart = 20 + m_pPattern->get_length() * m_fGridWidth;
		if ( (m_nRulerWidth - nXStart) != 0 ) {
			painter.fillRect( nXStart, 0, m_nRulerWidth - nXStart, m_nRulerHeight, QColor(170,170,170) );
		}
	}

	// numbers
	QColor textColor( 100, 100, 100 );
	QColor lineColor( 170, 170, 170 );

	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	painter.setFont(font);
	painter.drawLine( 0, 0, m_nRulerWidth, 0 );
	painter.drawLine( 0, m_nRulerHeight - 1, m_nRulerWidth - 1, m_nRulerHeight - 1);

	uint nQuarter = 48;

	for ( int i = 0; i < 64 ; i++ ) {
		int nText_x = 20 + nQuarter / 4 * i * m_fGridWidth;
		if ( ( i % 4 ) == 0 ) {
			painter.setPen( textColor );
			painter.drawText( nText_x - 30, 0, 60, m_nRulerHeight, Qt::AlignCenter, QString("%1").arg(i / 4 + 1) );
			//ERRORLOG(QString("nText_x: %1, true, : %2").arg(nText_x).arg(m_nRulerWidth));
		}
		else {
			painter.setPen( QPen( QColor( lineColor ), 1, Qt::SolidLine ) );
			painter.drawLine( nText_x, ( m_nRulerHeight - 5 ) / 2, nText_x, m_nRulerHeight - ( (m_nRulerHeight - 5 ) / 2 ));
			//ERRORLOG("PAINT LINE");
		}
	}

}


void PatternEditorRuler::paintEvent( QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}

	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackground->devicePixelRatio() ) {
		createBackground();
	}

	QPainter painter(this);

	painter.drawPixmap( ev->rect(), *m_pBackground, QRectF( pixelRatio * ev->rect().x(),
															pixelRatio * ev->rect().y(),
															pixelRatio * ev->rect().width(),
															pixelRatio * ev->rect().height() ) );

	// draw tickPosition
	if (m_nTicks != -1) {
		uint x = (uint)( 20 + m_nTicks * m_fGridWidth - 5 - 11 / 2.0 );
		painter.drawPixmap( QRect( x, height() / 2, 11, 8 ), m_tickPosition, QRect( 0, 0, 11, 8 ) );
	}
}



void PatternEditorRuler::zoomIn()
{
	if ( m_fGridWidth >= 3 ){
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
	m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
	resize(  QSize(m_nRulerWidth, m_nRulerHeight ));
	createBackground();
	update();
}


void PatternEditorRuler::zoomOut()
{
	if ( m_fGridWidth > 1.5 ) {
		if ( m_fGridWidth > 3 ){
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
		m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
		resize( QSize(m_nRulerWidth, m_nRulerHeight) );
		createBackground();
		update();
	}
}


void PatternEditorRuler::selectedPatternChangedEvent()
{
	createBackground();
	updateEditor( true );
}

void PatternEditorRuler::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		update( 0, 0, width(), height() );
	}
}
