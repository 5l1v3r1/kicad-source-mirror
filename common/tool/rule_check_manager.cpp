/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_rule_checker_control_base.h>
#include <tool/rule_check_manager.h>


RULE_CHECK_MANAGER_BASE::RULE_CHECK_MANAGER_BASE( const std::string& aName ) :
        TOOL_INTERACTIVE( aName ),
        m_controlDialog()
{
}


void RULE_CHECK_MANAGER_BASE::Reset( RESET_REASON aReason )
{

}


void RULE_CHECK_MANAGER_BASE::ShowControlDialog( wxWindow* aParent )
{
    if( !m_controlDialog )
        createControlDialog( aParent );

    m_controlDialog->Show( true );
}