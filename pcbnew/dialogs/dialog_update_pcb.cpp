/**
 * @file pcbnew/dialogs/dialog_update_pcb.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <dialog_update_pcb.h>
#include <wx_html_report_panel.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <class_board.h>
#include <ratsnest_data.h>
#include <kiface_i.h>

using namespace std::placeholders;


bool DIALOG_UPDATE_PCB::m_warnForNoNetPads = false;


DIALOG_UPDATE_PCB::DIALOG_UPDATE_PCB( PCB_EDIT_FRAME* aParent, NETLIST* aNetlist ) :
    DIALOG_UPDATE_PCB_BASE( aParent ),
    m_frame( aParent ),
    m_netlist( aNetlist ),
    m_initialized( false )
{
    auto cfg = m_frame->GetSettings();

    m_cbUpdateFootprints->SetValue( cfg->m_NetlistDialog.update_footprints );
    m_cbDeleteExtraFootprints->SetValue( cfg->m_NetlistDialog.delete_extra_footprints );
    m_cbDeleteSinglePadNets->SetValue( cfg->m_NetlistDialog.delete_single_pad_nets );
    m_cbWarnNoNetPad->SetValue( m_warnForNoNetPads );

    m_messagePanel->SetLabel( _("Changes To Be Applied") );
    m_messagePanel->SetLazyUpdate( true );
    m_netlist->SortByReference();

    m_messagePanel->SetVisibleSeverities( cfg->m_NetlistDialog.report_filter );

    m_messagePanel->GetSizer()->SetSizeHints( this );
    m_messagePanel->Layout();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Update PCB" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();
    FinishDialogSettings();

    m_initialized = true;
    PerformUpdate( true );
}


DIALOG_UPDATE_PCB::~DIALOG_UPDATE_PCB()
{
    m_warnForNoNetPads = m_cbWarnNoNetPad->GetValue();

    auto cfg = m_frame->GetSettings();

    cfg->m_NetlistDialog.update_footprints       = m_cbUpdateFootprints->GetValue();
    cfg->m_NetlistDialog.delete_extra_footprints = m_cbDeleteExtraFootprints->GetValue();
    cfg->m_NetlistDialog.delete_single_pad_nets  = m_cbDeleteSinglePadNets->GetValue();
    cfg->m_NetlistDialog.report_filter           = m_messagePanel->GetVisibleSeverities();

    if( m_runDragCommand )
    {
        KIGFX::VIEW_CONTROLS* controls = m_frame->GetCanvas()->GetViewControls();
        controls->SetCursorPosition( controls->GetMousePosition() );
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::move, true );
    }
}


void DIALOG_UPDATE_PCB::PerformUpdate( bool aDryRun )
{
    m_messagePanel->Clear();

    REPORTER& reporter = m_messagePanel->Reporter();

    m_runDragCommand = false;

    m_netlist->SetDeleteExtraFootprints( m_cbDeleteExtraFootprints->GetValue() );
    m_netlist->SetFindByTimeStamp( !m_cbRelinkFootprints->GetValue() );
    m_netlist->SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );

    BOARD_NETLIST_UPDATER updater( m_frame, m_frame->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun );
    updater.SetLookupByTimestamp( !m_cbRelinkFootprints->GetValue() );
    updater.SetDeleteUnusedComponents ( m_cbDeleteExtraFootprints->GetValue() );
    updater.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );
    updater.SetDeleteSinglePadNets( m_cbDeleteSinglePadNets->GetValue() );
    updater.SetWarnPadNoNetInNetlist( m_warnForNoNetPads );
    updater.UpdateNetlist( *m_netlist );

    m_messagePanel->Flush( true );

    if( aDryRun )
        return;

    m_frame->OnNetlistChanged( updater, &m_runDragCommand );
}


void DIALOG_UPDATE_PCB::OnOptionChanged( wxCommandEvent& event )
{
    if( m_initialized )
        PerformUpdate( true );
}


void DIALOG_UPDATE_PCB::OnUpdateClick( wxCommandEvent& event )
{
    m_messagePanel->SetLabel( _( "Changes Applied To PCB" ) );
    PerformUpdate( false );
    m_sdbSizer1Cancel->SetDefault();
}
