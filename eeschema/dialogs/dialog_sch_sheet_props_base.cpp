///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_sch_sheet_props_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_SHEET_PROPS_BASE::DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_longForm = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_grid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 4, 11 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 120 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 72 );
	m_grid->SetColSize( 4, 72 );
	m_grid->SetColSize( 5, 48 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelValue( 2, _("Show") );
	m_grid->SetColLabelValue( 3, _("H Align") );
	m_grid->SetColLabelValue( 4, _("V Align") );
	m_grid->SetColLabelValue( 5, _("Italic") );
	m_grid->SetColLabelValue( 6, _("Bold") );
	m_grid->SetColLabelValue( 7, _("Text Size") );
	m_grid->SetColLabelValue( 8, _("Orientation") );
	m_grid->SetColLabelValue( 9, _("X Position") );
	m_grid->SetColLabelValue( 10, _("Y Position") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( -1,180 ) );

	sbFields->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );
	m_bpAdd->SetMinSize( wxSize( 30,30 ) );

	bButtonSize->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );
	m_bpMoveUp->SetMinSize( wxSize( 30,30 ) );

	bButtonSize->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );
	m_bpMoveDown->SetMinSize( wxSize( 30,30 ) );

	bButtonSize->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 0, 0, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_bpDelete = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );
	m_bpDelete->SetMinSize( wxSize( 30,30 ) );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSize, 0, wxALL|wxEXPAND, 5 );


	m_longForm->Add( sbFields, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Style") ), wxHORIZONTAL );

	m_borderWidthLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Border width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	sbSizer2->Add( m_borderWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_borderWidthCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_borderWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_borderWidthUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	sbSizer2->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	sbSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_borderColorLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Border color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	sbSizer2->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_borderColorSwatch = new COLOR_SWATCH( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_borderColorSwatch->SetMinSize( wxSize( 48,24 ) );

	sbSizer2->Add( m_borderColorSwatch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer2->Add( 40, 0, 1, wxEXPAND, 5 );

	m_backgroundColorLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Background fill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorLabel->Wrap( -1 );
	sbSizer2->Add( m_backgroundColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_backgroundColorSwatch = new COLOR_SWATCH( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorSwatch->SetMinSize( wxSize( 48,24 ) );

	sbSizer2->Add( m_backgroundColorSwatch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer5->Add( sbSizer2, 1, wxEXPAND|wxBOTTOM, 5 );


	m_longForm->Add( bSizer5, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( m_longForm, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hiearchicalPathLabel = new wxStaticText( this, wxID_ANY, _("Hierarchical Path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hiearchicalPathLabel->Wrap( -1 );
	fgSizer1->Add( m_hiearchicalPathLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_heirarchyPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer1->Add( m_heirarchyPath, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_timestampLabel = new wxStaticText( this, wxID_ANY, _("Unique ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timestampLabel->Wrap( -1 );
	fgSizer1->Add( m_timestampLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_textCtrlTimeStamp = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlTimeStamp->SetToolTip( _("Unique ID that identifies the symbol") );

	fgSizer1->Add( m_textCtrlTimeStamp, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	bSizerBottom->Add( fgSizer1, 1, wxEXPAND|wxALL, 5 );


	bSizerBottom->Add( 100, 0, 0, wxEXPAND, 5 );

	m_stdDialogButtonSizer = new wxStdDialogButtonSizer();
	m_stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerOK );
	m_stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerCancel );
	m_stdDialogButtonSizer->Realize();

	bSizerBottom->Add( m_stdDialogButtonSizer, 0, wxEXPAND|wxALL, 5 );


	mainSizer->Add( bSizerBottom, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnDeleteField ), NULL, this );
}

DIALOG_SCH_SHEET_PROPS_BASE::~DIALOG_SCH_SHEET_PROPS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_SHEET_PROPS_BASE::OnDeleteField ), NULL, this );

}
