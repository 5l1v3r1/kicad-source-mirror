/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <gr_basic.h>
#include <kicad_string.h>
#include <richio.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <general.h>
#include <class_library.h>
#include <lib_rectangle.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_legacy_plugin.h>
#include <netlist_object.h>
#include <lib_item.h>
#include <symbol_lib_table.h>

#include <dialogs/dialog_schematic_find.h>

#include <wx/tokenzr.h>
#include <iostream>
#include <cctype>

#include <eeschema_id.h>    // for MAX_UNIT_COUNT_PER_PACKAGE definition

#include <trace_helpers.h>


/**
 * Convert a wxString to UTF8 and replace any control characters with a ~,
 * where a control character is one of the first ASCII values up to ' ' 32d.
 */
std::string toUTFTildaText( const wxString& txt )
{
    std::string ret = TO_UTF8( txt );

    for( std::string::iterator it = ret.begin();  it!=ret.end();  ++it )
    {
        if( (unsigned char) *it <= ' ' )
            *it = '~';
    }
    return ret;
}


/**
 * Used to draw a dummy shape when a LIB_PART is not found in library
 *
 * This component is a 400 mils square with the text ??
 * DEF DUMMY U 0 40 Y Y 1 0 N
 * F0 "U" 0 -350 60 H V
 * F1 "DUMMY" 0 350 60 H V
 * DRAW
 * T 0 0 0 150 0 0 0 ??
 * S -200 200 200 -200 0 1 0
 * ENDDRAW
 * ENDDEF
 */
static LIB_PART* dummy()
{
    static LIB_PART* part;

    if( !part )
    {
        part = new LIB_PART( wxEmptyString );

        LIB_RECTANGLE* square = new LIB_RECTANGLE( part );

        square->MoveTo( wxPoint( Mils2iu( -200 ), Mils2iu( 200 ) ) );
        square->SetEndPosition( wxPoint( Mils2iu( 200 ), Mils2iu( -200 ) ) );

        LIB_TEXT* text = new LIB_TEXT( part );

        text->SetTextSize( wxSize( Mils2iu( 150 ), Mils2iu( 150 ) ) );
        text->SetText( wxString( wxT( "??" ) ) );

        part->AddDrawItem( square );
        part->AddDrawItem( text );
    }

    return part;
}


SCH_COMPONENT::SCH_COMPONENT( const wxPoint& aPos, SCH_ITEM* aParent ) :
    SCH_ITEM( aParent, SCH_COMPONENT_T )
{
    Init( aPos );
}


SCH_COMPONENT::SCH_COMPONENT( LIB_PART& aPart, LIB_ID aLibId, SCH_SHEET_PATH* sheet,
                              int unit, int convert, const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_COMPONENT_T )
{
    Init( pos );

    m_unit      = unit;
    m_convert   = convert;
    m_lib_id    = aLibId;

    std::unique_ptr< LIB_PART > part;

    part = aPart.Flatten();
    part->SetParent();
    m_part.reset( part.release() );

    // Copy fields from the library component
    UpdateFields( true, true );

    // Update the pin locations
    UpdatePins();

    // Update the reference -- just the prefix for now.
    if( sheet )
        SetRef( sheet, aPart.GetReferenceField().GetText() + wxT( "?" ) );
    else
        m_prefix = aPart.GetReferenceField().GetText() + wxT( "?" );
}

SCH_COMPONENT::SCH_COMPONENT(
        LIB_PART& aPart, SCH_SHEET_PATH* aSheet, COMPONENT_SELECTION& aSel, const wxPoint& pos )
        : SCH_COMPONENT( aPart, aSel.LibId, aSheet, aSel.Unit, aSel.Convert, pos )
{
    // Set any fields that were modified as part of the component selection
    for( auto const& i : aSel.Fields )
    {
        auto field = this->GetField( i.first );

        if( field )
            field->SetText( i.second );
    }
}


SCH_COMPONENT::SCH_COMPONENT( const SCH_COMPONENT& aComponent ) :
    SCH_ITEM( aComponent )
{
    m_Parent      = aComponent.m_Parent;
    m_Pos         = aComponent.m_Pos;
    m_unit        = aComponent.m_unit;
    m_convert     = aComponent.m_convert;
    m_lib_id      = aComponent.m_lib_id;
    m_isInNetlist = aComponent.m_isInNetlist;

    if( aComponent.m_part )
        m_part.reset( new LIB_PART( *aComponent.m_part.get() ) );

    const_cast<KIID&>( m_Uuid ) = aComponent.m_Uuid;

    m_transform = aComponent.m_transform;
    m_prefix = aComponent.m_prefix;
    m_instanceReferences = aComponent.m_instanceReferences;
    m_Fields = aComponent.m_Fields;

    // Re-parent the fields, which before this had aComponent as parent
    for( SCH_FIELD& field : m_Fields )
        field.SetParent( this );

    UpdatePins();

    m_fieldsAutoplaced = aComponent.m_fieldsAutoplaced;
}


void SCH_COMPONENT::Init( const wxPoint& pos )
{
    m_Pos     = pos;
    m_unit    = 1;  // In multi unit chip - which unit to draw.
    m_convert = LIB_ITEM::LIB_CONVERT::BASE;  // De Morgan Handling

    // The rotation/mirror transformation matrix. pos normal
    m_transform = TRANSFORM();

    // construct only the mandatory fields, which are the first 4 only.
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        m_Fields.emplace_back( pos, i, this, TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) );

        if( i == REFERENCE )
            m_Fields.back().SetLayer( LAYER_REFERENCEPART );
        else if( i == VALUE )
            m_Fields.back().SetLayer( LAYER_VALUEPART );
        else
            m_Fields.back().SetLayer( LAYER_FIELDS );
    }

    m_prefix = wxString( wxT( "U" ) );
    m_isInNetlist = true;
}


EDA_ITEM* SCH_COMPONENT::Clone() const
{
    return new SCH_COMPONENT( *this );
}


void SCH_COMPONENT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 3;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_SELECTION_SHADOWS;
}


void SCH_COMPONENT::SetLibId( const LIB_ID& aLibId, PART_LIBS* aLibs )
{
    if( m_lib_id != aLibId )
    {
        m_lib_id = aLibId;
        SetModified();

        if( aLibs )
        {
            Resolve( aLibs );
        }
        else
        {
            m_part.reset();
            m_pins.clear();
            m_pinMap.clear();
        }
    }
}


void SCH_COMPONENT::SetLibId( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aSymLibTable,
                              PART_LIB* aCacheLib )
{
    if( m_lib_id == aLibId )
        return;

    m_lib_id = aLibId;
    SetModified();

    std::unique_ptr< LIB_PART > symbol;

    if( aSymLibTable && aSymLibTable->HasLibrary( m_lib_id.GetLibNickname() ) )
    {
        LIB_PART* tmp = aSymLibTable->LoadSymbol( m_lib_id );

        if( tmp )
        {
            symbol = tmp->Flatten();
            symbol->SetParent();
        }
    }

    if( !symbol && aCacheLib )
    {
        LIB_PART* tmp = aCacheLib->FindPart( m_lib_id.Format().wx_str() );

        if( tmp )
        {
            symbol = tmp->Flatten();
            symbol->SetParent();
        }
    }

    m_part.reset( symbol.release() );
    UpdatePins();
}


wxString SCH_COMPONENT::GetDescription() const
{
    if( m_part )
    {
        return m_part->GetDescription();
    }

    return wxEmptyString;
}


wxString SCH_COMPONENT::GetDatasheet() const
{
    if( m_part )
    {
        return m_part->GetDocFileName();
    }

    return wxEmptyString;
}


bool SCH_COMPONENT::Resolve( PART_LIBS* aLibs )
{
    // I've never been happy that the actual individual PART_LIB is left up to
    // flimsy search path ordering.  None-the-less find a part based on that design:
    if( LIB_PART* part = aLibs->FindLibPart( m_lib_id ) )
    {
        std::unique_ptr< LIB_PART > flattenedPart = part->Flatten();
        flattenedPart->SetParent();
        m_part.reset( flattenedPart.release() );
        UpdatePins();
        return true;
    }

    return false;
}


bool SCH_COMPONENT::Resolve( SYMBOL_LIB_TABLE& aLibTable, PART_LIB* aCacheLib )
{
    std::unique_ptr< LIB_PART > part;

    try
    {
        // We want a full symbol not just the top level child symbol.
        PROPERTIES props;

        props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";

        // LIB_TABLE_BASE::LoadSymbol() throws an IO_ERROR if the the library nickname
        // is not found in the table so check if the library still exists in the table
        // before attempting to load the symbol.
        if( m_lib_id.IsValid() && aLibTable.HasLibrary( m_lib_id.GetLibNickname() ) )
        {
            LIB_PART* tmp = aLibTable.LoadSymbol( m_lib_id );

            if( tmp )
            {
                part = tmp->Flatten();
                part->SetParent();
            }
        }

        // Fall back to cache library.  This is temporary until the new schematic file
        // format is implemented.
        if( !part && aCacheLib )
        {
            wxString libId = m_lib_id.Format().wx_str();
            libId.Replace( ":", "_" );
            wxLogTrace( traceSymbolResolver,
                        "Library symbol %s not found falling back to cache library.",
                        m_lib_id.Format().wx_str() );
            LIB_PART* tmp = aCacheLib->FindPart( libId );

            if( tmp )
            {
                part = tmp->Flatten();
                part->SetParent();
            }
        }

        if( part )
        {
            m_part.reset( part.release() );
            UpdatePins();
            return true;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogTrace( traceSymbolResolver, "I/O error %s resolving library symbol %s", ioe.What(),
                    m_lib_id.Format().wx_str() );
    }

    wxLogTrace( traceSymbolResolver, "Cannot resolve library symbol %s",
                m_lib_id.Format().wx_str() );

    m_part.reset();
    UpdatePins();     // This will clear the pin map and library symbol pin pointers.

    return false;
}


// Helper sort function, used in SCH_COMPONENT::ResolveAll, to sort sch component by lib_id
static bool sort_by_libid( const SCH_COMPONENT* ref, SCH_COMPONENT* cmp )
{
    if( ref->GetLibId() == cmp->GetLibId() )
    {
        if( ref->GetUnit() == cmp->GetUnit() )
            return ref->GetConvert() < cmp->GetConvert();

        return ref->GetUnit() < cmp->GetUnit();
    }

    return ref->GetLibId() < cmp->GetLibId();
}


void SCH_COMPONENT::ResolveAll(
        std::vector<SCH_COMPONENT*>& aComponents, SYMBOL_LIB_TABLE& aLibTable, PART_LIB* aCacheLib )
{
    // sort it by lib part. Cmp will be grouped by same lib part.
    std::sort( aComponents.begin(), aComponents.end(), sort_by_libid );

    LIB_ID curr_libid;

    for( unsigned ii = 0; ii < aComponents.size(); ++ii )
    {
        SCH_COMPONENT* cmp = aComponents[ii];
        curr_libid = cmp->m_lib_id;
        cmp->Resolve( aLibTable, aCacheLib );
        cmp->UpdatePins();

        // Propagate the m_part pointer to other members using the same lib_id
        for( unsigned jj = ii + 1; jj < aComponents.size(); ++jj )
        {
            SCH_COMPONENT* next_cmp = aComponents[jj];

            if( curr_libid != next_cmp->m_lib_id )
                break;

            if( cmp->m_part )
                next_cmp->m_part.reset( new LIB_PART( *cmp->m_part.get() ) );

            next_cmp->UpdatePins();

            ii = jj;
        }
    }
}


void SCH_COMPONENT::UpdatePins()
{
    m_pins.clear();
    m_pinMap.clear();

    if( m_part )
    {
        SCH_PIN_MAP map;
        unsigned i = 0;

        for( LIB_PIN* libPin = m_part->GetNextPin(); libPin; libPin = m_part->GetNextPin( libPin ) )
        {
            wxASSERT( libPin->Type() == LIB_PIN_T );

            if( libPin->GetConvert() && m_convert && ( m_convert != libPin->GetConvert() ) )
                continue;

            m_pins.push_back( std::unique_ptr<SCH_PIN>( new SCH_PIN( libPin, this ) ) );
            m_pinMap[ libPin ] = i;

            ++i;
        }
    }
}


SCH_CONNECTION* SCH_COMPONENT::GetConnectionForPin( LIB_PIN* aPin, const SCH_SHEET_PATH& aSheet )
{
    if( m_pinMap.count( aPin ) )
        return m_pins[ m_pinMap[aPin] ]->Connection( aSheet );

    return nullptr;
}


void SCH_COMPONENT::SetUnit( int aUnit )
{
    if( m_unit != aUnit )
    {
        m_unit = aUnit;
        SetModified();
    }
}


void SCH_COMPONENT::UpdateUnit( int aUnit )
{
    m_unit = aUnit;
}


void SCH_COMPONENT::SetConvert( int aConvert )
{
    if( m_convert != aConvert )
    {
        m_convert = aConvert;
        SetModified();
    }
}


void SCH_COMPONENT::SetTransform( const TRANSFORM& aTransform )
{
    if( m_transform != aTransform )
    {
        m_transform = aTransform;
        SetModified();
    }
}


int SCH_COMPONENT::GetUnitCount() const
{
    if( m_part )
        return m_part->GetUnitCount();

    return 0;
}


void SCH_COMPONENT::Print( wxDC* aDC, const wxPoint& aOffset )
{
    auto opts = PART_DRAW_OPTIONS::Default();
    opts.transform = m_transform;
    opts.draw_visible_fields = false;
    opts.draw_hidden_fields = false;

    if( m_part )
    {
        m_part->Print( aDC, m_Pos + aOffset, m_unit, m_convert, opts );
    }
    else    // Use dummy() part if the actual cannot be found.
    {
        dummy()->Print( aDC, m_Pos + aOffset, 0, 0, opts );
    }

    for( SCH_FIELD& field : m_Fields )
        field.Print( aDC, aOffset );
}


void SCH_COMPONENT::AddHierarchicalReference( const KIID_PATH& aPath, const wxString& aRef,
                                              int aUnit )
{
    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_instanceReferences.size(); ii++ )
    {
        if( m_instanceReferences[ii].m_Path == aPath )
        {
            m_instanceReferences.erase( m_instanceReferences.begin() + ii );
            ii--;
        }
    }

    COMPONENT_INSTANCE_REFERENCE instance;
    instance.m_Path = aPath;
    instance.m_Reference = aRef;
    instance.m_Unit = aUnit;
    m_instanceReferences.push_back( instance );
}


const wxString SCH_COMPONENT::GetRef( const SCH_SHEET_PATH* sheet, bool aIncludeUnit )
{
    KIID_PATH path = sheet->Path();
    wxString  ref;

    for( const COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            ref = instance.m_Reference;
            break;
        }
    }

    // If it was not found in m_Paths array, then see if it is in m_Field[REFERENCE] -- if so,
    // use this as a default for this path.  This will happen if we load a version 1 schematic
    // file.  It will also mean that multiple instances of the same sheet by default all have
    // the same component references, but perhaps this is best.
    if( ref.IsEmpty() && !GetField( REFERENCE )->GetText().IsEmpty() )
    {
        SetRef( sheet, GetField( REFERENCE )->GetText() );
        ref = GetField( REFERENCE )->GetText();
    }

    if( ref.IsEmpty() )
        ref = m_prefix;

    if( aIncludeUnit )
        ref += LIB_PART::SubReference( GetUnit() );

    return ref;
}


bool SCH_COMPONENT::IsReferenceStringValid( const wxString& aReferenceString )
{
    wxString text = aReferenceString;
    bool ok = true;

    // Try to unannotate this reference
    while( !text.IsEmpty() && ( text.Last() == '?' || wxIsdigit( text.Last() ) ) )
        text.RemoveLast();

    if( text.IsEmpty() )
        ok = false;

    return ok;
}


void SCH_COMPONENT::SetRef( const SCH_SHEET_PATH* sheet, const wxString& ref )
{
    KIID_PATH path = sheet->Path();
    bool      notInArray = true;

    // check to see if it is already there before inserting it
    for( COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Reference = ref;
            notInArray = false;
        }
    }

    if( notInArray )
        AddHierarchicalReference( path, ref, m_unit );

    SCH_FIELD* rf = GetField( REFERENCE );

    // @todo Should we really be checking for what is a "reasonable" position?
    if( rf->GetText().IsEmpty()
      || ( abs( rf->GetTextPos().x - m_Pos.x ) +
           abs( rf->GetTextPos().y - m_Pos.y ) > Mils2iu( 10000 ) ) )
    {
        // move it to a reasonable position
        rf->SetTextPos( m_Pos + wxPoint( Mils2iu( 50 ), Mils2iu( 50 ) ) );
    }

    rf->SetText( ref );  // for drawing.

    // Reinit the m_prefix member if needed
    wxString prefix = ref;

    if( IsReferenceStringValid( prefix ) )
    {
        while( prefix.Last() == '?' || wxIsdigit( prefix.Last() ) )
            prefix.RemoveLast();
    }
    else
    {
        prefix = wxT( "U" );        // Set to default ref prefix
    }

    if( m_prefix != prefix )
        m_prefix = prefix;

    // Power components have references starting with # and are not included in netlists
    m_isInNetlist = ! ref.StartsWith( wxT( "#" ) );
}


bool SCH_COMPONENT::IsAnnotated( const SCH_SHEET_PATH* aSheet )
{
    KIID_PATH path = aSheet->Path();

    for( const COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
            return instance.m_Reference.Last() != '?';
    }

    return false;
}


int SCH_COMPONENT::GetUnitSelection( const SCH_SHEET_PATH* aSheet ) const
{
    KIID_PATH path = aSheet->Path();

    for( const COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
            return instance.m_Unit;
    }

    // If it was not found in m_Paths array, then use m_unit.  This will happen if we load a
    // version 1 schematic file.
    return m_unit;
}


void SCH_COMPONENT::SetUnitSelection( const SCH_SHEET_PATH* aSheet, int aUnitSelection )
{
    KIID_PATH path = aSheet->Path();
    bool      notInArray = true;

    // check to see if it is already there before inserting it
    for( COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Unit = aUnitSelection;
            notInArray = false;
        }
    }

    if( notInArray )
        AddHierarchicalReference( path, m_prefix, aUnitSelection );
}


SCH_FIELD* SCH_COMPONENT::GetField( int aFieldNdx ) const
{
    const SCH_FIELD* field;

    if( (unsigned) aFieldNdx < m_Fields.size() )
        field = &m_Fields[aFieldNdx];
    else
        field = NULL;

    wxASSERT( field );

    return const_cast<SCH_FIELD*>( field );
}


wxString SCH_COMPONENT::GetFieldText( const wxString& aFieldName, SCH_EDIT_FRAME* aFrame ) const
{
    for( const SCH_FIELD& field : m_Fields )
    {
        if( aFieldName == field.GetName() )
            return field.GetText();
    }

    return wxEmptyString;
}


void SCH_COMPONENT::GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly )
{
    for( SCH_FIELD& field : m_Fields )
    {
        if( !aVisibleOnly || ( field.IsVisible() && !field.IsVoid() ) )
            aVector.push_back( &field );
    }
}


SCH_FIELD* SCH_COMPONENT::AddField( const SCH_FIELD& aField )
{
    int newNdx = m_Fields.size();

    m_Fields.push_back( aField );
    return &m_Fields[newNdx];
}


void SCH_COMPONENT::RemoveField( const wxString& aFieldName )
{
    for( unsigned i = MANDATORY_FIELDS; i < m_Fields.size(); ++i )
    {
        if( aFieldName == m_Fields[i].GetName( false ) )
        {
            m_Fields.erase( m_Fields.begin() + i );
            return;
        }
    }
}


SCH_FIELD* SCH_COMPONENT::FindField( const wxString& aFieldName, bool aIncludeDefaultFields )
{
    unsigned start = aIncludeDefaultFields ? 0 : MANDATORY_FIELDS;

    for( unsigned i = start; i < m_Fields.size(); ++i )
    {
        if( aFieldName == m_Fields[i].GetName( false ) )
            return &m_Fields[i];
    }

    return NULL;
}


void SCH_COMPONENT::UpdateFields( bool aResetStyle, bool aResetRef )
{
    if( m_part )
    {
        wxString symbolName;
        LIB_FIELDS fields;
        m_part->GetFields( fields );

        for( const LIB_FIELD& libField : fields )
        {
            int idx = libField.GetId();
            SCH_FIELD* schField;

            if( idx == REFERENCE && !aResetRef )
                continue;

            if( idx >= 0 && idx < MANDATORY_FIELDS )
                schField = GetField( idx );
            else
            {
                schField = FindField( libField.GetCanonicalName() );

                if( !schField )
                {
                    wxString  fieldName = libField.GetCanonicalName();
                    SCH_FIELD newField( wxPoint( 0, 0), GetFieldCount(), this, fieldName );
                    schField = AddField( newField );
                }
            }

            if( aResetStyle )
            {
                schField->ImportValues( libField );
                schField->SetTextPos( m_Pos + libField.GetTextPos() );
            }

            if( idx == VALUE )
            {
                schField->SetText( m_lib_id.GetLibItemName() ); // fetch alias-specific value
                symbolName = m_lib_id.GetLibItemName();
            }
            else if( idx == DATASHEET )
            {
                schField->SetText( GetDatasheet() );            // fetch alias-specific value

                // Some older libraries may be broken and the alias datasheet information
                // in the document file for the root part may have been dropped.  This only
                // happens for the root part.
                if( schField->GetText().IsEmpty() && symbolName == m_part->GetName() )
                    schField->SetText( m_part->GetField( DATASHEET )->GetText() );
            }
            else
            {
                schField->SetText( libField.GetText() );
            }
        }
    }
}


LIB_PIN* SCH_COMPONENT::GetPin( const wxString& number )
{
    if( m_part )
    {
        return m_part->GetPin( number, m_unit, m_convert );
    }

    return NULL;
}


void SCH_COMPONENT::GetPins( std::vector<LIB_PIN*>& aPinsList )
{
    if( m_part )
        m_part->GetPins( aPinsList, m_unit, m_convert );
}


SCH_PIN_PTRS SCH_COMPONENT::GetSchPins( const SCH_SHEET_PATH* aSheet ) const
{
    if( aSheet == nullptr )
        aSheet = g_CurrentSheet;

    // TODO(JE) if this works, consider caching in m_sheet_pins
    int unit = GetUnitSelection( aSheet );
    SCH_PIN_PTRS ptrs;

    for( const auto& p : m_pins )
    {
        if( unit && p->GetLibPin()->GetUnit() && ( p->GetLibPin()->GetUnit() != unit ) )
            continue;

        ptrs.push_back( p.get() );
    }

    return ptrs;
}


void SCH_COMPONENT::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_COMPONENT_T),
                 wxT( "Cannot swap data with invalid component." ) );

    SCH_COMPONENT* component = (SCH_COMPONENT*) aItem;

    std::swap( m_lib_id, component->m_lib_id );

    LIB_PART* part = component->m_part.release();
    component->m_part.reset( m_part.release() );
    component->UpdatePins();
    m_part.reset( part );
    UpdatePins();

    std::swap( m_Pos, component->m_Pos );
    std::swap( m_unit, component->m_unit );
    std::swap( m_convert, component->m_convert );

    m_Fields.swap( component->m_Fields );    // std::vector's swap()

    for( SCH_FIELD& field : component->m_Fields )
        field.SetParent( component );

    for( SCH_FIELD& field : m_Fields )
        field.SetParent( this );

    TRANSFORM tmp = m_transform;

    m_transform = component->m_transform;
    component->m_transform = tmp;

    std::swap( m_instanceReferences, component->m_instanceReferences );
}


bool SCH_COMPONENT::ResolveTextVar( wxString* token, int aDepth ) const
{
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        if( token->IsSameAs( m_Fields[ i ].GetCanonicalName().Upper() ) )
        {
            *token = m_Fields[ i ].GetShownText( aDepth + 1 );
            return true;
        }
    }

    for( size_t i = MANDATORY_FIELDS; i < m_Fields.size(); ++i )
    {
        if( token->IsSameAs( m_Fields[ i ].GetName() )
            || token->IsSameAs( m_Fields[ i ].GetName().Upper() ) )
        {
            *token = m_Fields[ i ].GetShownText( aDepth + 1 );
            return true;
        }
    }

    if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) ) )
    {
        const SCH_FIELD& field = m_Fields[ FOOTPRINT ];
        wxArrayString parts = wxSplit( field.GetText(), ':' );

        *token = parts[ 0 ];
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        const SCH_FIELD& field = m_Fields[ FOOTPRINT ];
        wxArrayString parts = wxSplit( field.GetText(), ':' );

        *token = parts[ std::min( 1, (int) parts.size() - 1 ) ];
        return true;
    }
    else if( token->IsSameAs( wxT( "UNIT" ) ) )
    {
        *token = LIB_PART::SubReference( GetUnit() );
        return true;
    }

    return false;
}


void SCH_COMPONENT::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{
    // Build a reference with no annotation,
    // i.e. a reference ended by only one '?'
    wxString defRef = m_prefix;

    if( !IsReferenceStringValid( defRef ) )
    {   // This is a malformed reference: reinit this reference
        m_prefix = defRef = wxT("U");        // Set to default ref prefix
    }

    while( defRef.Last() == '?' )
        defRef.RemoveLast();

    defRef.Append( wxT( "?" ) );

    if( aSheetPath )
    {
        KIID_PATH path = aSheetPath->Path();

        for( COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
        {
            if( instance.m_Path == path )
                instance.m_Reference = defRef;
        }
    }
    else
    {
        for( COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
            instance.m_Reference = defRef;
    }

    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    m_Fields[REFERENCE].SetText( defRef ); //for drawing.

    SetModified();
}


bool SCH_COMPONENT::AddSheetPathReferenceEntryIfMissing( const KIID_PATH& aSheetPath )
{
    // a empty sheet path is illegal:
    wxCHECK( aSheetPath.size() > 0, false );

    wxString reference_path;

    for( const COMPONENT_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        // if aSheetPath is found, nothing to do:
        if( instance.m_Path == aSheetPath )
            return false;
    }

    // This entry does not exist: add it, with its last-used reference
    AddHierarchicalReference( aSheetPath, m_Fields[REFERENCE].GetText(), m_unit );
    return true;
}


void SCH_COMPONENT::SetOrientation( int aOrientation )
{
    TRANSFORM temp = TRANSFORM();
    bool transform = false;

    switch( aOrientation )
    {
    case CMP_ORIENT_0:
    case CMP_NORMAL:                    // default transform matrix
        m_transform.x1 = 1;
        m_transform.y2 = -1;
        m_transform.x2 = m_transform.y1 = 0;
        break;

    case CMP_ROTATE_COUNTERCLOCKWISE:  // Rotate + (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = 1;
        temp.x2   = -1;
        transform = true;
        break;

    case CMP_ROTATE_CLOCKWISE:          // Rotate - (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = -1;
        temp.x2   = 1;
        transform = true;
        break;

    case CMP_MIRROR_Y:                  // Mirror Y (incremental rotation)
        temp.x1   = -1;
        temp.y2   = 1;
        temp.y1   = temp.x2 = 0;
        transform = true;
        break;

    case CMP_MIRROR_X:                  // Mirror X (incremental rotation)
        temp.x1   = 1;
        temp.y2   = -1;
        temp.y1   = temp.x2 = 0;
        transform = true;
        break;

    case CMP_ORIENT_90:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_180:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_270:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_CLOCKWISE );
        break;

    case ( CMP_ORIENT_0 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_0 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_90 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_90 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_90 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_90 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_180 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_180 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_180 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_180 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_270 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_270 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_270 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_270 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    default:
        transform = false;
        wxMessageBox( wxT( "SetRotateMiroir() error: ill value" ) );
        break;
    }

    if( transform )
    {
        /* The new matrix transform is the old matrix transform modified by the
         *  requested transformation, which is the temp transform (rot,
         *  mirror ..) in order to have (in term of matrix transform):
         *     transform coord = new_m_transform * coord
         *  where transform coord is the coord modified by new_m_transform from
         *  the initial value coord.
         *  new_m_transform is computed (from old_m_transform and temp) to
         *  have:
         *     transform coord = old_m_transform * temp
         */
        TRANSFORM newTransform;

        newTransform.x1 = m_transform.x1 * temp.x1 + m_transform.x2 * temp.y1;
        newTransform.y1 = m_transform.y1 * temp.x1 + m_transform.y2 * temp.y1;
        newTransform.x2 = m_transform.x1 * temp.x2 + m_transform.x2 * temp.y2;
        newTransform.y2 = m_transform.y1 * temp.x2 + m_transform.y2 * temp.y2;
        m_transform = newTransform;
    }
}


int SCH_COMPONENT::GetOrientation()
{
    int rotate_values[] =
    {
        CMP_ORIENT_0,
        CMP_ORIENT_90,
        CMP_ORIENT_180,
        CMP_ORIENT_270,
        CMP_MIRROR_X + CMP_ORIENT_0,
        CMP_MIRROR_X + CMP_ORIENT_90,
        CMP_MIRROR_X + CMP_ORIENT_270,
        CMP_MIRROR_Y,
        CMP_MIRROR_Y + CMP_ORIENT_0,
        CMP_MIRROR_Y + CMP_ORIENT_90,
        CMP_MIRROR_Y + CMP_ORIENT_180,
        CMP_MIRROR_Y + CMP_ORIENT_270
    };

    // Try to find the current transform option:
    TRANSFORM transform = m_transform;

    for( int type_rotate : rotate_values )
    {
        SetOrientation( type_rotate );

        if( transform == m_transform )
            return type_rotate;
    }

    // Error: orientation not found in list (should not happen)
    wxMessageBox( wxT( "Component orientation matrix internal error" ) );
    m_transform = transform;

    return CMP_NORMAL;
}


#if defined(DEBUG)

void SCH_COMPONENT::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " ref=\"" << TO_UTF8( GetField( 0 )->GetName() )
                                 << '"' << " chipName=\""
                                 << GetLibId().Format() << '"' << m_Pos
                                 << " layer=\"" << m_Layer
                                 << '"' << ">\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->GetText();

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\""
                                             << TO_UTF8( GetField( i )->GetName() )
                                             << '"' << " value=\""
                                             << TO_UTF8( value ) << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << TO_UTF8( GetClass().Lower() ) << ">\n";
}

#endif


EDA_RECT SCH_COMPONENT::GetBodyBoundingBox() const
{
    EDA_RECT    bBox;

    if( m_part )
    {
        bBox = m_part->GetBodyBoundingBox( m_unit, m_convert );
    }
    else
    {
        bBox = dummy()->GetBodyBoundingBox( m_unit, m_convert );
    }

    int x0 = bBox.GetX();
    int xm = bBox.GetRight();

    // We must reverse Y values, because matrix orientation
    // suppose Y axis normal for the library items coordinates,
    // m_transform reverse Y values, but bBox is already reversed!
    int y0 = -bBox.GetY();
    int ym = -bBox.GetBottom();

    // Compute the real Boundary box (rotated, mirrored ...)
    int x1 = m_transform.x1 * x0 + m_transform.y1 * y0;
    int y1 = m_transform.x2 * x0 + m_transform.y2 * y0;
    int x2 = m_transform.x1 * xm + m_transform.y1 * ym;
    int y2 = m_transform.x2 * xm + m_transform.y2 * ym;

    bBox.SetX( x1 );
    bBox.SetY( y1 );
    bBox.SetWidth( x2 - x1 );
    bBox.SetHeight( y2 - y1 );
    bBox.Normalize();

    bBox.Offset( m_Pos );
    return bBox;
}


const EDA_RECT SCH_COMPONENT::GetBoundingBox() const
{
    EDA_RECT bbox = GetBodyBoundingBox();

    for( size_t i = 0; i < m_Fields.size(); i++ )
        bbox.Merge( m_Fields[i].GetBoundingBox() );

    return bbox;
}


void SCH_COMPONENT::GetMsgPanelInfo( EDA_UNITS aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    // part and alias can differ if alias is not the root
    if( m_part )
    {
        if( m_part.get() != dummy() )
        {
            if( g_CurrentSheet )
                aList.push_back( MSG_PANEL_ITEM( _( "Reference" ), GetRef( g_CurrentSheet ),
                                                 DARKCYAN ) );

            msg = m_part->IsPower() ? _( "Power symbol" ) : _( "Value" );

            aList.push_back( MSG_PANEL_ITEM( msg, GetField( VALUE )->GetShownText(), DARKCYAN ) );

            // Display component reference in library and library
            aList.push_back( MSG_PANEL_ITEM( _( "Name" ), GetLibId().GetLibItemName(), BROWN ) );

            if( !m_part->IsRoot() )
            {
                msg = _( "Missing parent" );

                std::shared_ptr< LIB_PART > parent = m_part->GetParent().lock();

                if( parent )
                    msg = parent->GetName();

                aList.push_back( MSG_PANEL_ITEM( _( "Alias of" ), msg, BROWN ) );
            }

            if( m_part->GetLib() && m_part->GetLib()->IsCache() )
                aList.push_back( MSG_PANEL_ITEM( _( "Library" ),
                                                 m_part->GetLib()->GetLogicalName(), RED ) );
            else if( !m_lib_id.GetLibNickname().empty() )
                aList.push_back( MSG_PANEL_ITEM( _( "Library" ), m_lib_id.GetLibNickname(),
                                                 BROWN ) );
            else
                aList.push_back( MSG_PANEL_ITEM( _( "Library" ), _( "Undefined!!!" ), RED ) );

            // Display the current associated footprint, if exists.
            if( !GetField( FOOTPRINT )->IsVoid() )
                msg = GetField( FOOTPRINT )->GetShownText();
            else
                msg = _( "<Unknown>" );

            aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), msg, DARKRED ) );

            // Display description of the component, and keywords found in lib
            aList.push_back( MSG_PANEL_ITEM( _( "Description" ), m_part->GetDescription(),
                                             DARKCYAN ) );
            aList.push_back( MSG_PANEL_ITEM( _( "Key words" ), m_part->GetKeyWords(), DARKCYAN ) );
        }
    }
    else
    {
        if( g_CurrentSheet )
            aList.push_back( MSG_PANEL_ITEM( _( "Reference" ), GetRef( g_CurrentSheet ),
                                             DARKCYAN ) );

        aList.push_back( MSG_PANEL_ITEM( _( "Value" ), GetField( VALUE )->GetShownText(),
                                         DARKCYAN ) );
        aList.push_back( MSG_PANEL_ITEM( _( "Name" ), GetLibId().GetLibItemName(), BROWN ) );

        wxString libNickname = GetLibId().GetLibNickname();

        if( libNickname.empty() )
        {
            aList.push_back( MSG_PANEL_ITEM( _( "Library" ), _( "No library defined!" ), RED ) );
        }
        else
        {
            msg.Printf( _( "Symbol not found in %s!" ), libNickname );
            aList.push_back( MSG_PANEL_ITEM( _( "Library" ), msg , RED ) );
        }
    }
}


BITMAP_DEF SCH_COMPONENT::GetMenuImage() const
{
    return add_component_xpm;
}


void SCH_COMPONENT::MirrorY( int aYaxis_position )
{
    int dx = m_Pos.x;

    SetOrientation( CMP_MIRROR_Y );
    MIRROR( m_Pos.x, aYaxis_position );
    dx -= m_Pos.x;     // dx,0 is the move vector for this transform

    for( SCH_FIELD& field : m_Fields )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = field.GetTextPos();
        pos.x -= dx;
        field.SetTextPos( pos );
    }
}


void SCH_COMPONENT::MirrorX( int aXaxis_position )
{
    int dy = m_Pos.y;

    SetOrientation( CMP_MIRROR_X );
    MIRROR( m_Pos.y, aXaxis_position );
    dy -= m_Pos.y;     // dy,0 is the move vector for this transform

    for( SCH_FIELD& field : m_Fields )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = field.GetTextPos();
        pos.y -= dy;
        field.SetTextPos( pos );
    }
}


void SCH_COMPONENT::Rotate( wxPoint aPosition )
{
    wxPoint prev = m_Pos;

    RotatePoint( &m_Pos, aPosition, 900 );

    SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );

    for( SCH_FIELD& field : m_Fields )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = field.GetTextPos();
        pos.x -= prev.x - m_Pos.x;
        pos.y -= prev.y - m_Pos.y;
        field.SetTextPos( pos );
    }
}


bool SCH_COMPONENT::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

    // Components are searchable via the child field and pin item text.
    return false;
}


void SCH_COMPONENT::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    if( m_part )
    {
        for( LIB_PIN* pin = m_part->GetNextPin();  pin;  pin = m_part->GetNextPin( pin ) )
        {
            wxASSERT( pin->Type() == LIB_PIN_T );

            if( pin->GetUnit() && m_unit && ( m_unit != pin->GetUnit() ) )
                continue;

            if( pin->GetConvert() && m_convert && ( m_convert != pin->GetConvert() ) )
                continue;

            DANGLING_END_ITEM item( PIN_END, pin, GetPinPhysicalPosition( pin ), this );
            aItemList.push_back( item );
        }
    }
}


bool SCH_COMPONENT::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                         const SCH_SHEET_PATH* aPath )
{
    bool changed = false;

    for( auto& pin : m_pins )
    {
        bool previousState = pin->IsDangling();
        pin->SetIsDangling( true );

        wxPoint pos = m_transform.TransformCoordinate( pin->GetPosition() ) + m_Pos;

        for( DANGLING_END_ITEM& each_item : aItemList )
        {
            // Some people like to stack pins on top of each other in a symbol to indicate
            // internal connection. While technically connected, it is not particularly useful
            // to display them that way, so skip any pins that are in the same symbol as this
            // one.
            if( each_item.GetParent() == this )
                continue;

            switch( each_item.GetType() )
            {
            case PIN_END:
            case LABEL_END:
            case SHEET_LABEL_END:
            case WIRE_START_END:
            case WIRE_END_END:
            case NO_CONNECT_END:
            case JUNCTION_END:

                if( pos == each_item.GetPosition() )
                    pin->SetIsDangling( false );

                break;

            default:
                break;
            }

            if( !pin->IsDangling() )
                break;
        }

        changed = ( changed || ( previousState != pin->IsDangling() ) );
    }

    return changed;
}


wxPoint SCH_COMPONENT::GetPinPhysicalPosition( const LIB_PIN* Pin ) const
{
    wxCHECK_MSG( Pin != NULL && Pin->Type() == LIB_PIN_T, wxPoint( 0, 0 ),
                 wxT( "Cannot get physical position of pin." ) );

    return m_transform.TransformCoordinate( Pin->GetPosition() ) + m_Pos;
}


void SCH_COMPONENT::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    for( const auto& pin : m_pins )
    {
        // Collect only pins attached to the current unit and convert.
        // others are not associated to this component instance
        int pin_unit = pin.get()->GetLibPin()->GetUnit();
        int pin_convert = pin.get()->GetLibPin()->GetConvert();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_convert > 0 && pin_convert != GetConvert() )
            continue;

        aPoints.push_back( m_transform.TransformCoordinate( pin->GetPosition() ) + m_Pos );
    }
}


LIB_ITEM* SCH_COMPONENT::GetDrawItem( const wxPoint& aPosition, KICAD_T aType )
{
    UpdatePins();

    if( m_part )
    {
        // Calculate the position relative to the component.
        wxPoint libPosition = aPosition - m_Pos;

        return m_part->LocateDrawItem( m_unit, m_convert, aType, libPosition, m_transform );
    }

    return NULL;
}


wxString SCH_COMPONENT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Symbol %s, %s" ),
                             GetLibId().GetLibItemName().wx_str(),
                             GetField( REFERENCE )->GetShownText() );
}


SEARCH_RESULT SCH_COMPONENT::Visit( INSPECTOR aInspector, void* aTestData,
                                    const KICAD_T aFilterTypes[] )
{
    KICAD_T     stype;

    for( const KICAD_T* p = aFilterTypes; (stype = *p) != EOT; ++p )
    {
        // If caller wants to inspect component type or and component children types.
        if( stype == SCH_LOCATE_ANY_T || stype == Type() )
        {
            if( SEARCH_RESULT::QUIT == aInspector( this, aTestData ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_FIELD_T )
        {
            // Test the bounding boxes of fields if they are visible and not empty.
            for( SCH_FIELD& field : m_Fields )
            {
                if( SEARCH_RESULT::QUIT == aInspector( &field, (void*) this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }

        if( stype == SCH_FIELD_LOCATE_REFERENCE_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( REFERENCE ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_VALUE_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( VALUE ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_FOOTPRINT_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( FOOTPRINT ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_DATASHEET_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( DATASHEET ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_PIN_T )
        {
            for( auto& pin : m_pins )
            {
                // Collect only pins attached to the current unit and convert.
                // others are not associated to this component instance
                int pin_unit = pin.get()->GetLibPin()->GetUnit();
                int pin_convert = pin.get()->GetLibPin()->GetConvert();

                if( pin_unit > 0 && pin_unit != GetUnit() )
                    continue;

                if( pin_convert > 0 && pin_convert != GetConvert() )
                    continue;

                if( SEARCH_RESULT::QUIT == aInspector( pin.get(), (void*) this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }
    }

    return SEARCH_RESULT::CONTINUE;
}


void SCH_COMPONENT::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                    SCH_SHEET_PATH*      aSheetPath )
{
    if( m_part )
    {
        for( LIB_PIN* pin = m_part->GetNextPin();  pin;  pin = m_part->GetNextPin( pin ) )
        {
            wxASSERT( pin->Type() == LIB_PIN_T );

            if( pin->GetUnit() && ( pin->GetUnit() != GetUnitSelection( aSheetPath ) ) )
                continue;

            if( pin->GetConvert() && ( pin->GetConvert() != GetConvert() ) )
                continue;

            wxPoint pos = GetTransform().TransformCoordinate( pin->GetPosition() ) + m_Pos;

            NETLIST_OBJECT* item = new NETLIST_OBJECT();
            item->m_SheetPathInclude = *aSheetPath;
            item->m_Comp = m_pins[ m_pinMap.at( pin ) ].get();
            item->m_SheetPath = *aSheetPath;
            item->m_Type = NETLIST_ITEM::PIN;
            item->m_Link = (SCH_ITEM*) this;
            item->m_ElectricalPinType = pin->GetType();
            item->m_PinNum = pin->GetNumber();
            item->m_Label = pin->GetName();
            item->m_Start = item->m_End = pos;

            aNetListItems.push_back( item );

            if( pin->IsPowerConnection() )
            {
                // There is an associated PIN_LABEL.
                item = new NETLIST_OBJECT();
                item->m_SheetPathInclude = *aSheetPath;
                item->m_Comp = NULL;
                item->m_SheetPath = *aSheetPath;
                item->m_Type = NETLIST_ITEM::PINLABEL;
                item->m_Label = pin->GetName();
                item->m_Start = pos;
                item->m_End = item->m_Start;

                aNetListItems.push_back( item );
            }
        }
    }
}


bool SCH_COMPONENT::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto component = static_cast<const SCH_COMPONENT*>( &aItem );

    EDA_RECT rect = GetBodyBoundingBox();

    if( rect.GetArea() != component->GetBodyBoundingBox().GetArea() )
        return rect.GetArea() < component->GetBodyBoundingBox().GetArea();

    if( m_Pos.x != component->m_Pos.x )
        return m_Pos.x < component->m_Pos.x;

    if( m_Pos.y != component->m_Pos.y )
        return m_Pos.y < component->m_Pos.y;

    return m_Uuid < aItem.m_Uuid;       // Ensure deterministic sort
}


bool SCH_COMPONENT::operator==( const SCH_COMPONENT& aComponent ) const
{
    if( GetFieldCount() !=  aComponent.GetFieldCount() )
        return false;

    for( int i = VALUE; i < GetFieldCount(); i++ )
    {
        if( GetField( i )->GetText().Cmp( aComponent.GetField( i )->GetText() ) != 0 )
            return false;
    }

    return true;
}


bool SCH_COMPONENT::operator!=( const SCH_COMPONENT& aComponent ) const
{
    return !( *this == aComponent );
}


SCH_COMPONENT& SCH_COMPONENT::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_COMPONENT* c = (SCH_COMPONENT*) &aItem;

        m_lib_id    = c->m_lib_id;

        LIB_PART* libSymbol = c->m_part ? new LIB_PART( *c->m_part.get() ) : nullptr;

        m_part.reset( libSymbol );
        m_Pos       = c->m_Pos;
        m_unit      = c->m_unit;
        m_convert   = c->m_convert;
        m_transform = c->m_transform;

        m_instanceReferences = c->m_instanceReferences;

        m_Fields    = c->m_Fields;    // std::vector's assignment operator

        // Reparent fields after assignment to new component.
        for( SCH_FIELD& field : m_Fields )
            field.SetParent( this );

        UpdatePins();
    }

    return *this;
}


bool SCH_COMPONENT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBodyBoundingBox();
    bBox.Inflate( aAccuracy );

    if( bBox.Contains( aPosition ) )
        return true;

    return false;
}


bool SCH_COMPONENT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_Flags & STRUCT_DELETED || m_Flags & SKIP_STRUCT )
        return false;

    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


bool SCH_COMPONENT::doIsConnected( const wxPoint& aPosition ) const
{
    wxPoint new_pos = m_transform.InverseTransform().TransformCoordinate( aPosition - m_Pos );

    for( const auto& pin : m_pins )
    {
        // Collect only pins attached to the current unit and convert.
        // others are not associated to this component instance
        int pin_unit = pin.get()->GetLibPin()->GetUnit();
        int pin_convert = pin.get()->GetLibPin()->GetConvert();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_convert > 0 && pin_convert != GetConvert() )
            continue;

        if( pin->GetPosition() == new_pos )
            return true;
    }

    return false;
}


bool SCH_COMPONENT::IsInNetlist() const
{
    return m_isInNetlist;
}


void SCH_COMPONENT::Plot( PLOTTER* aPlotter )
{
    TRANSFORM temp;

    if( m_part )
    {
        temp = GetTransform();
        aPlotter->StartBlock( nullptr );

        m_part->Plot( aPlotter, GetUnit(), GetConvert(), m_Pos, temp );

        for( SCH_FIELD field : m_Fields )
            field.Plot( aPlotter );

        aPlotter->EndBlock( nullptr );
    }
}


bool SCH_COMPONENT::HasBrightenedPins()
{
    for( const auto& pin : m_pins )
    {
        if( pin->IsBrightened() )
            return true;
    }

    return false;
}


void SCH_COMPONENT::ClearBrightenedPins()
{
    for( auto& pin : m_pins )
        pin->ClearBrightened();
}


void SCH_COMPONENT::BrightenPin( LIB_PIN* aPin )
{
    if( m_pinMap.count( aPin ) )
        m_pins[ m_pinMap.at( aPin ) ]->SetBrightened();
}


void SCH_COMPONENT::ClearHighlightedPins()
{
    for( auto& pin : m_pins )
        pin->ClearHighlighted();
}


bool SCH_COMPONENT::HasHighlightedPins()
{
    for( const auto& pin : m_pins )
    {
        if( pin->IsHighlighted() )
            return true;
    }

    return false;
}


void SCH_COMPONENT::HighlightPin( LIB_PIN* aPin )
{
    if( m_pinMap.count( aPin ) )
        m_pins[ m_pinMap.at( aPin ) ]->SetHighlighted();
}


bool SCH_COMPONENT::ClearAllHighlightFlags()
{
    bool retVal = false;

    if( IsHighlighted() )
    {
        ClearFlags( HIGHLIGHTED );
        retVal = true;
    }

    // Clear the HIGHLIGHTED flag of pins
    if( HasHighlightedPins() )
    {
        ClearHighlightedPins();
        retVal = true;
    }

    // Clear the HIGHLIGHTED flag of other items, currently only fields
    for( SCH_FIELD& each_field : m_Fields )
    {
        if( each_field.IsHighlighted() )
        {
            each_field.ClearFlags( HIGHLIGHTED );
            retVal = true;
        }
    }

    return retVal;
}
