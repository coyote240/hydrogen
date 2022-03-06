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

#include "AdsrTest.h"

#include <core/Basics/Adsr.h>
#include <stdio.h>
#include <memory>

using namespace H2Core;

const double delta = 0.00001;

void ADSRTest::setUp()
{
	m_adsr = std::make_shared<ADSR>( 1.0, 2.0, 0.8, 256.0 );
}

float ADSRTest::getValue( float fStep ) {
	float fL = 1.0, fR = 1.0;
	m_adsr->applyADSR( &fL, &fR, 1, 2, fStep );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( fL, fR, delta );
	return fL;
}

void ADSRTest::testAttack()
{
	m_adsr->attack();

	/* Attack */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.774681627750397, getValue( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, getValue( 0.1 ), delta );

	/* Decay */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, getValue( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.830416560173035, getValue( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 1.0 ), delta );

	/* Sustain */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 4.0 ), delta );
}


void ADSRTest::testRelease()
{
	getValue( 1.1 ); // move past Attack
	getValue( 2.1 ); // move past Decay
	getValue( 0.1 ); // calculate and store sustain

	/* Release note, and check if it was on sustain value */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->release(), delta );

	/* Release */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.121666193008423, getValue( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 128.0 ), delta );

	/* Idle */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 2.0 ), delta );
}
