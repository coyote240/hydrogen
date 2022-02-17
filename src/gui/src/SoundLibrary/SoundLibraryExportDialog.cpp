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

#include "SoundLibraryExportDialog.h"

#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>
#include <core/Preferences.h>
#include <core/H2Exception.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/DrumkitComponent.h>

#include <QFileDialog>
#include <QtGui>
#include <QtWidgets>

#include <memory>

#if defined(H2CORE_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <cstdio>
#endif

using namespace H2Core;

const char* SoundLibraryExportDialog::__class_name = "SoundLibraryExportDialog";

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent,  const QString& sSelectedKit, H2Core::Filesystem::Lookup lookup )
	: QDialog( pParent )
	, Object( __class_name )
	, m_sPreselectedKit( sSelectedKit )
	, m_preselectedKitLookup( lookup )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( tr( "Export Sound Library" ) );
	m_sSysDrumkitSuffix = " (system)";
	updateDrumkitList();
	adjustSize();
	setFixedSize( width(), height() );
	drumkitPathTxt->setText( QDir::homePath() );
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
	INFOLOG( "DESTROY" );

	for (uint i = 0; i < m_pDrumkitInfoList.size(); i++ ) {
		Drumkit* info = m_pDrumkitInfoList[i];
		delete info;
	}
	m_pDrumkitInfoList.clear();
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	bool bRecentVersion = versionList->currentIndex() == 1 ? false : true;
	
	Filesystem::Lookup lookup;
	bool bIsUserDrumkit;
	QString	sDrumkitName = drumkitList->currentText();
	if ( sDrumkitName.contains( m_sSysDrumkitSuffix ) ) {
		lookup = Filesystem::Lookup::system;
		sDrumkitName.replace( m_sSysDrumkitSuffix, "" );
	} else {
		lookup = Filesystem::Lookup::user;
	}
	auto pDrumkit = Drumkit::load_by_name( sDrumkitName, false,
										   lookup );

	if ( pDrumkit == nullptr ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Hydrogen",
							   tr("Unable to retrieve drumkit from sound library" ) );
		return;
	}
	
	if ( ! pDrumkit->exportTo( drumkitPathTxt->text(), // Target folder
							   componentList->currentText(), // Selected component
							   bRecentVersion ) ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Hydrogen", tr("Unable to export drumkit") );

		delete pDrumkit;
		return;
	}

	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen", tr("Drumkit exported.") );

	delete pDrumkit;
}

void SoundLibraryExportDialog::on_drumkitPathTxt_textChanged( QString str )
{
	QString path = drumkitPathTxt->text();
	if (path.isEmpty()) {
		exportBtn->setEnabled( false );
	}
	else {
		exportBtn->setEnabled( true );
	}
}

void SoundLibraryExportDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();
	QString filename = QFileDialog::getExistingDirectory (this, tr("Directory"), lastUsedDir);
	if ( filename.isEmpty() ) {
		drumkitPathTxt->setText( QDir::homePath() );
	}
	else
	{
		drumkitPathTxt->setText( filename );
		lastUsedDir = filename;
	}
}

void SoundLibraryExportDialog::on_cancelBtn_clicked()
{
	accept();
}

void SoundLibraryExportDialog::on_drumkitList_currentIndexChanged( QString str )
{
	componentList->clear();

	QStringList p_compoList = m_kit_components[str];

	for (QStringList::iterator it = p_compoList.begin() ; it != p_compoList.end(); ++it) {
		QString p_compoName = *it;

		componentList->addItem( p_compoName );
	}
}

void SoundLibraryExportDialog::on_versionList_currentIndexChanged( int index )
{
	if( index == 0 ) {
		componentList->setEnabled( false );
	} else if( index == 1 ) {
		componentList->setEnabled(  true );
	}
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	INFOLOG( "[updateDrumkitList]" );

	drumkitList->clear();

	for ( auto pDrumkitInfo : m_pDrumkitInfoList ) {
		delete pDrumkitInfo;
	}
	m_pDrumkitInfoList.clear();

	QStringList sysDrumkits = Filesystem::sys_drumkit_list();
	QString sDrumkitName;
	for (int i = 0; i < sysDrumkits.size(); ++i) {
		QString absPath = Filesystem::sys_drumkits_dir() + sysDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath, false );
		if (info) {
			m_pDrumkitInfoList.push_back( info );
			sDrumkitName = info->get_name() + m_sSysDrumkitSuffix;
			drumkitList->addItem( sDrumkitName );
			QStringList p_components;
			for ( auto pComponent : *(info->get_components() ) ) {
				p_components.append( pComponent->get_name() );
			}
			m_kit_components[ sDrumkitName ] = p_components;
		}
	}

	drumkitList->insertSeparator( drumkitList->count() );

	QStringList userDrumkits = Filesystem::usr_drumkit_list();
	for (int i = 0; i < userDrumkits.size(); ++i) {
		QString absPath = Filesystem::usr_drumkits_dir() + userDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath, false );
		if (info) {
			m_pDrumkitInfoList.push_back( info );
			drumkitList->addItem( info->get_name() );
			QStringList p_components;
			for ( auto pComponent : *(info->get_components() ) ) {
				p_components.append(pComponent->get_name());
			}
			m_kit_components[info->get_name()] = p_components;
		}
	}

	/*
	 * If the export dialog was called from the soundlibrary panel via right click on
	 * a soundlibrary, the variable preselectedKit holds the name of the selected drumkit
	 */
	if ( m_preselectedKitLookup == Filesystem::Lookup::system ) {
		m_sPreselectedKit.append( m_sSysDrumkitSuffix );
	}

	int index = drumkitList->findText( m_sPreselectedKit );
	if ( index >= 0) {
		drumkitList->setCurrentIndex( index );
	}
	else {
		drumkitList->setCurrentIndex( 0 );
	}

	on_drumkitList_currentIndexChanged( drumkitList->currentText() );
	on_versionList_currentIndexChanged( 0 );
}
