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

#include "Button.h"

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#include <core/Globals.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>
#include <core/Hydrogen.h>

Button::Button( QWidget *pParent, QSize size, Type type, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, QString sBaseTooltip, bool bColorful, bool bModifyOnChange )
	: QPushButton( pParent )
	, m_size( size )
	, m_iconSize( iconSize )
	, m_sBaseTooltip( sBaseTooltip )
	, m_sRegisteredMidiEvent( "" )
	, m_nRegisteredMidiParameter( 0 )
	, m_bColorful( bColorful )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
	, m_bIsActive( true )
	, m_bUseRedBackground( bUseRedBackground )
	, m_nFixedFontSize( -1 )
	, m_bModifyOnChange( bModifyOnChange )
{
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFocusPolicy( Qt::NoFocus );
	
	if ( size.isNull() || size.isEmpty() ) {
		m_size = sizeHint();
	}
	adjustSize();
	setFixedSize( m_size );
	resize( m_size );

	if ( ! sIcon.isEmpty() ) {
		updateIcon();
	} else {
		setText( sText );
	}

	if ( size.width() <= 12 || size.height() <= 12 ) {
		m_sBorderRadius = "0";
	} else if ( size.width() <= 20 || size.height() <= 20 ) {
		m_sBorderRadius = "3";
	} else {
		m_sBorderRadius = "5";
	}
	
	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	updateFont();
	updateStyleSheet();
	updateTooltip();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Button::onPreferencesChanged );

	if ( type == Type::Toggle ) {
		connect( this, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
	} else {
		connect( this, SIGNAL(clicked(bool)), this, SLOT(onToggled(bool)));
	}
}

Button::~Button() {
}

void Button::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	
	setEnabled( bIsActive );

	update();
}


void Button::updateIcon() {
	if ( m_bColorful ) {
		setIcon( QIcon( Skin::getSvgImagePath() + "/icons/" + m_sIcon ) );
	} else {
		if ( H2Core::Preferences::get_instance()->getIconColor() ==
			 H2Core::InterfaceTheme::IconColor::White ) {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/white/" + m_sIcon ) );
		} else {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + m_sIcon ) );
		}
	}
	setIconSize( m_iconSize );
}

void Button::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	
	int nFactorGradient = 126;
	int nFactorGradientShadow = 225;
	int nHover = 12;
	float fStop1 = 0.2;
	float fStop2 = 0.85;
	float x1 = 0;
	float x2 = 1;
	float y1 = 0;
	float y2 = 1;
	
	QColor backgroundLight = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient );
	QColor backgroundDark = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient );
	QColor backgroundLightHover = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient + nHover );
	QColor backgroundDarkHover = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient + nHover );
	QColor backgroundShadowLight = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradientShadow );
	QColor backgroundShadowDark = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradientShadow );
	QColor backgroundShadowLightHover = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradientShadow + nHover );
	QColor backgroundShadowDarkHover = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradientShadow + nHover );
	QColor border = Qt::black;

	QColor backgroundCheckedLight, backgroundCheckedDark,
		backgroundCheckedLightHover, backgroundCheckedDarkHover,
		backgroundShadowCheckedLight, backgroundShadowCheckedDark,
		backgroundShadowCheckedLightHover, backgroundShadowCheckedDarkHover,
		textChecked;
	if ( ! m_bUseRedBackground ) {
		backgroundCheckedLight = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient );
		backgroundCheckedDark = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient );
		backgroundCheckedLightHover = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient + nHover );
		backgroundCheckedDarkHover = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient + nHover );
		backgroundShadowCheckedLight = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradientShadow );
		backgroundShadowCheckedDark = pPref->getColorTheme()->m_accentColor.darker( nFactorGradientShadow );
		backgroundShadowCheckedLightHover = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradientShadow + nHover );
		backgroundShadowCheckedDarkHover = pPref->getColorTheme()->m_accentColor.darker( nFactorGradientShadow + nHover );
		textChecked = pPref->getColorTheme()->m_accentTextColor;
	} else {
		backgroundCheckedLight = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradient );
		backgroundCheckedDark = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradient );
		backgroundCheckedLightHover = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradient + nHover );
		backgroundCheckedDarkHover = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradient + nHover );
		backgroundShadowCheckedLight = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradientShadow );
		backgroundShadowCheckedDark = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradientShadow );
		backgroundShadowCheckedLightHover = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradientShadow + nHover );
		backgroundShadowCheckedDarkHover = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradientShadow + nHover );
		textChecked = pPref->getColorTheme()->m_buttonRedTextColor;
	}

	QColor textColor = pPref->getColorTheme()->m_widgetTextColor;
	
	QColor backgroundInactiveLight =
		Skin::makeWidgetColorInactive( backgroundLight );
	QColor backgroundInactiveLightHover = backgroundInactiveLight;
	QColor backgroundInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundCheckedLight );
	QColor backgroundInactiveCheckedLightHover = backgroundInactiveCheckedLight;
	QColor backgroundInactiveDark =
		Skin::makeWidgetColorInactive( backgroundDark );
	QColor backgroundInactiveDarkHover = backgroundInactiveDark;
	QColor backgroundInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundCheckedDark );
	QColor backgroundInactiveCheckedDarkHover = backgroundInactiveCheckedDark;
	QColor backgroundShadowInactiveLight =
		Skin::makeWidgetColorInactive( backgroundShadowLight );
	QColor backgroundShadowInactiveLightHover = backgroundShadowInactiveLight;
	QColor backgroundShadowInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedLight );
	QColor backgroundShadowInactiveCheckedLightHover = backgroundShadowInactiveCheckedLight;
	QColor backgroundShadowInactiveDark =
		Skin::makeWidgetColorInactive( backgroundShadowDark );
	QColor backgroundShadowInactiveDarkHover = backgroundShadowInactiveDark;
	QColor backgroundShadowInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedDark );
	QColor backgroundShadowInactiveCheckedDarkHover = backgroundShadowInactiveCheckedDark;
	QColor textInactiveColor = Skin::makeTextColorInactive( textColor );
	
	setStyleSheet( QString( "\
QPushButton:enabled { \
    color: %1; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %23, stop: %39 %3, \
                                      stop: %40 %4, stop: 1 %24); \
} \
QPushButton:enabled:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %25, stop: %39 %5, \
                                      stop: %40 %6, stop: 1 %26); \
} \
QPushButton:enabled:checked { \
    color: %7; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %27, stop: %39 %8, \
                                      stop: %40 %9, stop: 1 %28); \
} \
QPushButton:enabled:checked:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %29, stop: %39 %10, \
                                      stop: %40 %11, stop: 1 %30); \
} \
QPushButton:disabled { \
    color: %13; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %31, stop: %39 %14, \
                                      stop: %40 %15, stop: 1 %32); \
} \
QPushButton:disabled:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %33, stop: %39 %16, \
                                      stop: %40 %17, stop: 1 %34); \
} \
QPushButton:disabled:checked { \
    color: %18; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %35, stop: %39 %19, \
                                      stop: %40 %20, stop: 1 %36); \
} \
QPushButton:disabled:checked:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %37, stop: %39 %21, \
                                      stop: %40 %22, stop: 1 %38); \
}"
							)
				   .arg( textColor.name() )
				   .arg( m_sBorderRadius )
				   .arg( backgroundLight.name() )
				   .arg( backgroundDark.name() )
				   .arg( backgroundLightHover.name() )
				   .arg( backgroundDarkHover.name() )
				   .arg( textChecked.name() )
				   .arg( backgroundCheckedLight.name() )
				   .arg( backgroundCheckedDark.name() )
				   .arg( backgroundCheckedLightHover.name() )
				   .arg( backgroundCheckedDarkHover.name() )
				   .arg( border.name() )
				   .arg( textInactiveColor.name() )
				   .arg( backgroundInactiveLight.name() )
				   .arg( backgroundInactiveDark.name() )
				   .arg( backgroundInactiveLightHover.name() )
				   .arg( backgroundInactiveDarkHover.name() )
				   .arg( textChecked.name() )
				   .arg( backgroundInactiveCheckedLight.name() )
				   .arg( backgroundInactiveCheckedDark.name() )
				   .arg( backgroundInactiveCheckedLightHover.name() )
				   .arg( backgroundInactiveCheckedDarkHover.name() )
				   .arg( backgroundShadowLight.name() )
				   .arg( backgroundShadowDark.name() )
				   .arg( backgroundShadowLightHover.name() )
				   .arg( backgroundShadowDarkHover.name() )
				   .arg( backgroundShadowCheckedLight.name() )
				   .arg( backgroundShadowCheckedDark.name() )
				   .arg( backgroundShadowCheckedLightHover.name() )
				   .arg( backgroundShadowCheckedDarkHover.name() )
				   .arg( backgroundShadowInactiveLight.name() )
				   .arg( backgroundShadowInactiveDark.name() )
				   .arg( backgroundShadowInactiveLightHover.name() )
				   .arg( backgroundShadowInactiveDarkHover.name() )
				   .arg( backgroundShadowInactiveCheckedLight.name() )
				   .arg( backgroundShadowInactiveCheckedDark.name() )
				   .arg( backgroundShadowInactiveCheckedLightHover.name() )
				   .arg( backgroundShadowInactiveCheckedDarkHover.name() )
				   .arg( fStop1 ).arg( fStop2 ).arg( x1 ).arg( x2 )
				   .arg( y1 ).arg( y2 ) );
}

void Button::setBaseToolTip( const QString& sNewTip ) {
	m_sBaseTooltip = sNewTip;
	updateTooltip();
}

void Button::setAction( std::shared_ptr<Action> pAction ) {
	m_pAction = pAction;
	updateTooltip();
}

void Button::mousePressEvent(QMouseEvent*ev) {

	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();

		// Store the registered MIDI event and parameter in order to
		// show them in the tooltip. Looking them up in the MidiMap
		// using the Action associated to the Widget might not yield a
		// unique result since the Action can be registered from the
		// PreferencesDialog as well.
		m_sRegisteredMidiEvent = H2Core::Hydrogen::get_instance()->m_LastMidiEvent;
		m_nRegisteredMidiParameter = H2Core::Hydrogen::get_instance()->m_nLastMidiEventParameter;
		
		updateTooltip();
		return;
	}

	QPushButton::mousePressEvent( ev );
}

void Button::updateTooltip() {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QString sTip = QString("%1" ).arg( m_sBaseTooltip );

	// Add the associated MIDI action.
	if ( m_pAction != nullptr ) {
		sTip.append( QString( "\n%1: %2 " ).arg( pCommonStrings->getMidiTooltipHeading() )
					 .arg( m_pAction->getType() ) );
		if ( ! m_sRegisteredMidiEvent.isEmpty() ) {
			sTip.append( QString( "%1 [%2 : %3]" ).arg( pCommonStrings->getMidiTooltipBound() )
						 .arg( m_sRegisteredMidiEvent ).arg( m_nRegisteredMidiParameter ) );
		} else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiTooltipUnbound() ) );
		}
	}
			
	setToolTip( sTip );
}

void Button::setSize( QSize size ) {
	m_size = size;
	
	adjustSize();
	if ( ! size.isNull() ) {
		setFixedSize( size );
		resize( size );
	}

	updateFont();
}

void Button::updateFont() {

	auto pPref = H2Core::Preferences::get_instance();
	
	float fScalingFactor = 1.0;
    switch ( pPref->getFontSize() ) {
    case H2Core::FontTheme::FontSize::Small:
		fScalingFactor = 1.2;
		break;
    case H2Core::FontTheme::FontSize::Normal:
		fScalingFactor = 1.0;
		break;
    case H2Core::FontTheme::FontSize::Large:
		fScalingFactor = 0.75;
		break;
	}

	int nPixelSize;
	if ( m_nFixedFontSize < 0 ) {

		int nMargin;
		if ( m_size.width() <= 12 || m_size.height() <= 12 ) {
			nMargin = 1;
		} else if ( m_size.width() <= 19 || m_size.height() <= 19 ) {
			nMargin = 5;
		} else if ( m_size.width() <= 22 || m_size.height() <= 22 ) {
			nMargin = 7;
		} else {
			nMargin = 9;
		}
	
		if ( m_size.width() >= m_size.height() ) {
			nPixelSize = m_size.height() - std::round( fScalingFactor * nMargin );
		} else {
			nPixelSize = m_size.width() - std::round( fScalingFactor * nMargin );
		}
	} else {
		nPixelSize = m_nFixedFontSize;
	}

	QFont font( pPref->getLevel3FontFamily() );
	font.setPixelSize( nPixelSize );
	setFont( font );

	if ( m_size.width() > m_size.height() ) {
		// Check whether the width of the text fits the available frame
		// width of the button.
		while ( fontMetrics().size( Qt::TextSingleLine, text() ).width() > width()
				&& nPixelSize > 1 ) {
			nPixelSize--;
			font.setPixelSize( nPixelSize );
			setFont( font );
		}
	}
}
	
void Button::paintEvent( QPaintEvent* ev )
{
	QPushButton::paintEvent( ev );

	updateFont();

	// Grey-out the widget some more if it is not enabled
	if ( ! isEnabled() ) {
		QPainter( this ).fillRect( ev->rect(), QColor( 128, 128, 128, 48 ) );
	}

}

void Button::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		updateFont();
		updateStyleSheet();
	}

	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcon();
	}
}

void Button::onToggled( bool bChecked ) {
	if ( m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}
