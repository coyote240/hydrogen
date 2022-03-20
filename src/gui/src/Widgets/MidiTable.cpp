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

#include "../Skin.h"
#include "LCDCombo.h"
#include "LCDSpinBox.h"
#include "MidiSenseWidget.h"
#include "MidiTable.h"

#include <core/MidiMap.h>
#include <core/Preferences/Preferences.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/Basics/InstrumentComponent.h>

#include <QHeaderView>

MidiTable::MidiTable( QWidget *pParent )
	: QTableWidget( pParent )
	, m_nRowHeight( 29 )
	, m_nColumn0Width( 25 )
	, m_nColumn1Width( 155 )
	, m_nColumn2Width( 76 )
	, m_nColumn3Width( 175 )
	, m_nColumn4Width( 60 )
	, m_nColumn5Width( 60 )
	, m_nColumn6Width( 65 )
 {
	m_nRowCount = 0;
	setupMidiTable();

	m_pUpdateTimer = new QTimer( this );
	m_nCurrentMidiAutosenseRow = 0;
}


MidiTable::~MidiTable()
{
	for( int myRow = 0; myRow <=  m_nRowCount ; myRow++ ) {
		delete cellWidget( myRow, 0 );
		delete cellWidget( myRow, 1 );
		delete cellWidget( myRow, 2 );
		delete cellWidget( myRow, 3 );
		delete cellWidget( myRow, 4 );
		delete cellWidget( myRow, 5 );
		delete cellWidget( myRow, 6 );
	}
}

void MidiTable::midiSensePressed( int row ){

	m_nCurrentMidiAutosenseRow = row;
	MidiSenseWidget midiSenseWidget( this );
	midiSenseWidget.exec();

	LCDCombo * eventCombo =  dynamic_cast <LCDCombo *> ( cellWidget( row, 1 ) );
	LCDSpinBox * eventSpinner = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 2 ) );


	eventCombo->setCurrentIndex( eventCombo->findText( midiSenseWidget.m_sLastMidiEvent ) );
	eventSpinner->setValue( midiSenseWidget.m_LastMidiEventParameter );

	m_pUpdateTimer->start( 100 );	
}

// Reimplementing this one is quite expensive. But the visibility of
// the spinBoxes is reset after the end of updateTable(). In addition,
// the function is only called frequently when interacting the the
// table via mouse. This won't happen too often.
void MidiTable::paintEvent( QPaintEvent* ev ) {
	QTableWidget::paintEvent( ev );
	updateTable();
}

void MidiTable::updateTable() {
	if( m_nRowCount > 0 ) {
		// Ensure that the last row is empty
		LCDCombo* pEventCombo =  dynamic_cast<LCDCombo*>( cellWidget( m_nRowCount - 1, 1 ) );
		LCDCombo* pActionCombo = dynamic_cast<LCDCombo*>( cellWidget( m_nRowCount - 1, 3 ) );

		if ( pEventCombo == nullptr || pActionCombo == nullptr ) {
			return;
		}

		if( ! pActionCombo->currentText().isEmpty() && ! pEventCombo->currentText().isEmpty() ) {
			std::shared_ptr<Action> pAction = std::make_shared<Action>();
			insertNewRow( pAction, "", 0 );
		}

		// Ensure that all other empty rows are removed and that the
		// parameter spinboxes are only shown when required for that
		// particular parameter.
		for ( int ii = 0; ii < m_nRowCount; ii++ ) {
			updateRow( ii );
		}
	}
}


void MidiTable::insertNewRow(std::shared_ptr<Action> pAction, QString eventString, int eventParameter)
{
	MidiActionManager *pActionHandler = MidiActionManager::get_instance();

	insertRow( m_nRowCount );
	
	int oldRowCount = m_nRowCount;

	++m_nRowCount;

	QPushButton *midiSenseButton = new QPushButton(this);
	midiSenseButton->setIcon(QIcon(Skin::getSvgImagePath() + "/icons/record.svg"));
	midiSenseButton->setIconSize( QSize( 13, 13 ) );
	midiSenseButton->setToolTip( tr("press button to record midi event") );

	QSignalMapper *signalMapper = new QSignalMapper(this);

	connect(midiSenseButton, SIGNAL( clicked()), signalMapper, SLOT( map() ));
	signalMapper->setMapping( midiSenseButton, oldRowCount );
	connect( signalMapper, SIGNAL(mapped( int ) ), this, SLOT( midiSensePressed(int) ) );
	setCellWidget( oldRowCount, 0, midiSenseButton );



	LCDCombo *eventBox = new LCDCombo(this);
	eventBox->setSize( QSize( m_nColumn1Width, m_nRowHeight ) );
	eventBox->insertItems( oldRowCount , pActionHandler->getEventList() );
	eventBox->setCurrentIndex( eventBox->findText(eventString) );
	connect( eventBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	setCellWidget( oldRowCount, 1, eventBox );
	
	
	LCDSpinBox *eventParameterSpinner = new LCDSpinBox(this);
	eventParameterSpinner->setSize( QSize( m_nColumn2Width, m_nRowHeight ) );
	setCellWidget( oldRowCount , 2, eventParameterSpinner );
	eventParameterSpinner->setMaximum( 999 );
	eventParameterSpinner->setValue( eventParameter );


	LCDCombo *actionBox = new LCDCombo(this);
	actionBox->setSize( QSize( m_nColumn3Width, m_nRowHeight ) );
	actionBox->insertItems( oldRowCount, pActionHandler->getActionList());
	actionBox->setCurrentIndex ( actionBox->findText( pAction->getType() ) );
	connect( actionBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	setCellWidget( oldRowCount , 3, actionBox );

	bool ok;
	LCDSpinBox *actionParameterSpinner1 = new LCDSpinBox(this);
	actionParameterSpinner1->setSize( QSize( m_nColumn4Width, m_nRowHeight ) );
	setCellWidget( oldRowCount , 4, actionParameterSpinner1 );
	actionParameterSpinner1->setMaximum( 999 );
	actionParameterSpinner1->setValue( pAction->getParameter1().toInt(&ok,10) );
	actionParameterSpinner1->hide();

	LCDSpinBox *actionParameterSpinner2 = new LCDSpinBox(this);
	actionParameterSpinner2->setSize( QSize( m_nColumn5Width, m_nRowHeight ) );
	setCellWidget( oldRowCount , 5, actionParameterSpinner2 );
	actionParameterSpinner2->setMaximum( std::max(MAX_FX, MAX_COMPONENTS) );
	actionParameterSpinner2->setValue( pAction->getParameter2().toInt(&ok,10) );
	actionParameterSpinner2->hide();

	LCDSpinBox *actionParameterSpinner3 = new LCDSpinBox(this);
	actionParameterSpinner3->setSize( QSize( m_nColumn6Width, m_nRowHeight ) );
	setCellWidget( oldRowCount , 6, actionParameterSpinner3 );
	actionParameterSpinner3->setMaximum( H2Core::InstrumentComponent::getMaxLayers() );
	actionParameterSpinner3->setValue( pAction->getParameter3().toInt(&ok,10) );
	actionParameterSpinner3->hide();
}

void MidiTable::setupMidiTable()
{
	MidiMap *pMidiMap = MidiMap::get_instance();

	QStringList items;
	items << "" << tr("Incoming Event")  << tr("Event Para.")
		  << tr("Action") <<  tr("Para. 1") << tr("Para. 2") << tr("Para. 3");

	setRowCount( 0 );
	setColumnCount( 7 );

	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );
	horizontalHeader()->setStretchLastSection(true);

	setColumnWidth( 0 , m_nColumn0Width );
	setColumnWidth( 1 , m_nColumn1Width );
	setColumnWidth( 2, m_nColumn2Width );
	setColumnWidth( 3, m_nColumn3Width );
	setColumnWidth( 4 , m_nColumn4Width );
	setColumnWidth( 5 , m_nColumn5Width );
	setColumnWidth( 6 , m_nColumn6Width );

	for( const auto& it : pMidiMap->getMMCMap() ) {
		insertNewRow( it.second, it.first, 0 );
	}

	for( int note = 0; note < 128; note++ ) {
		std::shared_ptr<Action> pAction = pMidiMap->getNoteAction( note );

		if ( pAction->getType() == "NOTHING" ){
			continue;
		}

		insertNewRow(pAction , "NOTE" , note );
	}

	for( int parameter = 0; parameter < 128; parameter++ ){
		std::shared_ptr<Action> pAction = pMidiMap->getCCAction( parameter );

		if ( pAction->getType() == "NOTHING" ){
			continue;
		}

		insertNewRow( pAction , "CC" , parameter );
	}

	{
		std::shared_ptr<Action> pAction = pMidiMap->getPCAction();
		if ( pAction->getType() != "NOTHING" ) {

			insertNewRow( pAction, "PROGRAM_CHANGE", 0 );
		}
	}

	std::shared_ptr<Action> pAction = std::make_shared<Action>();
	insertNewRow( pAction, "", 0 );
}


void MidiTable::saveMidiTable()
{
	MidiMap *mM = MidiMap::get_instance();
	
	for ( int row = 0; row < m_nRowCount; row++ ) {

		LCDCombo * eventCombo =  dynamic_cast <LCDCombo *> ( cellWidget( row, 1 ) );
		LCDSpinBox * eventSpinner = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 2 ) );
		LCDCombo * actionCombo = dynamic_cast <LCDCombo *> ( cellWidget( row, 3 ) );
		LCDSpinBox * actionSpinner1 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 4 ) );
		LCDSpinBox * actionSpinner2 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 5 ) );
		LCDSpinBox * actionSpinner3 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 6 ) );

		QString eventString;
		QString actionString;

		if( !eventCombo->currentText().isEmpty() && !actionCombo->currentText().isEmpty() ){
			eventString = eventCombo->currentText();

			actionString = actionCombo->currentText();
		
			std::shared_ptr<Action> pAction = std::make_shared<Action>( actionString );

			if( actionSpinner1->cleanText() != ""){
				pAction->setParameter1( actionSpinner1->cleanText() );
			}
			if( actionSpinner2->cleanText() != ""){
				pAction->setParameter2( actionSpinner2->cleanText() );
			}
			if( actionSpinner3->cleanText() != ""){
				pAction->setParameter3( actionSpinner3->cleanText() );
			}

			if( eventString.left(2) == "CC" ){
				mM->registerCCEvent( eventSpinner->cleanText().toInt() , pAction );
			} else if( eventString.left(3) == "MMC" ){
				mM->registerMMCEvent( eventString , pAction );
			} else if( eventString.left(4) == "NOTE" ){
				mM->registerNoteEvent( eventSpinner->cleanText().toInt() , pAction );
			} else if( eventString.left(14) == "PROGRAM_CHANGE" ){
				mM->registerPCEvent( pAction );
			}
		}
	}
}

void MidiTable::updateRow( int nRow ) {
	LCDCombo* pEventCombo =  dynamic_cast <LCDCombo*>( cellWidget( nRow, 1 ) );
	LCDCombo* pActionCombo = dynamic_cast <LCDCombo*>( cellWidget( nRow, 3 ) );

	if ( pEventCombo == nullptr || pActionCombo == nullptr ) {
		return;
	}

	if( pActionCombo->currentText().isEmpty() &&
		pEventCombo->currentText().isEmpty() && nRow != m_nRowCount - 1 ) {

		removeRow( nRow );
		m_nRowCount--;
		return;
	}

	QString sActionType = pActionCombo->currentText();
	LCDSpinBox* pActionSpinner1 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 4 ) );
	LCDSpinBox* pActionSpinner2 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 5 ) );
	LCDSpinBox* pActionSpinner3 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 6 ) );
	if ( sActionType == "NOTHING" || sActionType.isEmpty() ) {
		pActionSpinner1->hide();
		pActionSpinner2->hide();
		pActionSpinner3->hide();

	} else {
		int nParameterNumber = MidiActionManager::get_instance()->getParameterNumber( sActionType );
		if ( nParameterNumber != -1 ) {
			if ( nParameterNumber < 3 ) {
				pActionSpinner3->hide();
			} else {
				pActionSpinner3->show();
			}
			if ( nParameterNumber < 2 ) {
				pActionSpinner2->hide();
			} else {
				pActionSpinner2->show();
			}
			if ( nParameterNumber < 1 ) {
				pActionSpinner1->hide();
			} else {
				pActionSpinner1->show();
			}
		} else {
			ERRORLOG( QString( "Unable to find MIDI action [%1]" ).arg( sActionType ) );
		}
	}
}
