/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_edit_frame.h>
#include <lib_draw_item.h>
#include <general.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>


void SCH_EDIT_FRAME::GetSchematicConnections( std::vector< wxPoint >& aConnections )
{
    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        // Avoid items that are changing
        if( !( item->GetEditFlags() & ( IS_DRAGGED | IS_MOVED | IS_DELETED ) ) )
            item->GetConnectionPoints( aConnections );
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( aConnections.begin(), aConnections.end(),
            []( const wxPoint& a, const wxPoint& b ) -> bool
            { return a.x < b.x || (a.x == b.x && a.y < b.y); } );
    aConnections.erase( unique( aConnections.begin(), aConnections.end() ), aConnections.end() );
}


bool SCH_EDIT_FRAME::TestDanglingEnds()
{
    std::vector<DANGLING_END_ITEM> endPoints;
    bool hasStateChanged = false;

    for( SCH_ITEM* item = GetScreen()->GetDrawList().begin(); item; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( SCH_ITEM* item = GetScreen()->GetDrawList().begin(); item; item = item->Next() )
    {
        if( item->UpdateDanglingState( endPoints ) )
        {
            GetCanvas()->GetView()->Update( item, KIGFX::REPAINT );
            hasStateChanged = true;
        }
    }

    return hasStateChanged;
}


bool SCH_EDIT_FRAME::TrimWire( const wxPoint& aStart, const wxPoint& aEnd, bool aAppend )
{
    SCH_LINE* line;
    SCH_ITEM* next_item = NULL;
    bool retval = false;

    if( aStart == aEnd )
        return retval;

    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = next_item )
    {
        next_item = item->Next();

        // Don't remove wires that are already deleted or are currently being dragged
        if( item->GetEditFlags() & ( STRUCT_DELETED | IS_DRAGGED | IS_MOVED | SKIP_STRUCT ) )
            continue;

        if( item->Type() != SCH_LINE_T || item->GetLayer() != LAYER_WIRE )
            continue;

        line = (SCH_LINE*) item;
        if( !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aStart ) ||
                !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aEnd ) )
        {
            continue;
        }

        // Don't remove entire wires
        if( ( line->GetStartPoint() == aStart && line->GetEndPoint() == aEnd )
            || ( line->GetStartPoint() == aEnd && line->GetEndPoint() == aStart ) )
        {
            continue;
        }

        // Step 1: break the segment on one end.  return_line remains line if not broken.
        // Ensure that *line points to the segment containing aEnd
        SCH_LINE* return_line = line;
        aAppend |= BreakSegment( line, aStart, aAppend, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aEnd ) )
            line = return_line;

        // Step 2: break the remaining segment.  return_line remains line if not broken.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        aAppend |= BreakSegment( line, aEnd, aAppend, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aStart ) )
            line = return_line;

        SaveCopyInUndoList( (SCH_ITEM*)line, UR_DELETED, aAppend );
        RemoveFromScreen( (SCH_ITEM*)line );

        aAppend = true;
        retval = true;
    }

    return retval;
}


bool SCH_EDIT_FRAME::SchematicCleanUp( bool aUndo, SCH_SCREEN* aScreen )
{
    SCH_ITEM*           item = NULL;
    SCH_ITEM*           secondItem = NULL;
    PICKED_ITEMS_LIST   itemList;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    auto remove_item = [ &itemList ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        itemList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
    };

    BreakSegmentsOnJunctions( true, aScreen );

    for( item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( ( item->Type() != SCH_LINE_T )
            && ( item->Type() != SCH_JUNCTION_T )
            && ( item->Type() != SCH_NO_CONNECT_T ) )
            continue;

        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        // Remove unneeded junctions
        if( ( item->Type() == SCH_JUNCTION_T )
            && ( !aScreen->IsJunctionNeeded( item->GetPosition() ) ) )
        {
            remove_item( item );
            continue;
        }

        // Remove zero-length lines
        if( item->Type() == SCH_LINE_T
            && ( (SCH_LINE*) item )->IsNull() )
        {
            remove_item( item );
            continue;
        }

        for( secondItem = item->Next(); secondItem; secondItem = secondItem->Next() )
        {
            if( item->Type() != secondItem->Type()
              || ( secondItem->GetEditFlags() & STRUCT_DELETED ) )
                continue;

            // Merge overlapping lines
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE*   firstLine   = (SCH_LINE*) item;
                SCH_LINE*   secondLine  = (SCH_LINE*) secondItem;
                SCH_LINE*   line = NULL;
                bool needed = false;

                if( !secondLine->IsParallel( firstLine )
                  || secondLine->GetLineStyle() != firstLine->GetLineStyle()
                  || secondLine->GetLineColor() != firstLine->GetLineColor()
                  || secondLine->GetLineSize() != firstLine->GetLineSize() )
                    continue;

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                    && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( secondItem );
                    continue;
                }

                // If the end points overlap, check if we still need the junction
                if( secondLine->IsEndPoint( firstLine->GetStartPoint() ) )
                    needed = aScreen->IsJunctionNeeded( firstLine->GetStartPoint() );
                else if( secondLine->IsEndPoint( firstLine->GetEndPoint() ) )
                    needed = aScreen->IsJunctionNeeded( firstLine->GetEndPoint() );

                if( !needed && ( line = (SCH_LINE*) secondLine->MergeOverlap( firstLine ) ) )
                {
                    remove_item( item );
                    remove_item( secondItem );
                    itemList.PushItem( ITEM_PICKER( line, UR_NEW ) );
                    AddToScreen( line, aScreen );
                    break;
                }
            }
            // Remove duplicate junctions and no-connects
            else if( secondItem->GetPosition() == item->GetPosition() )
                remove_item( secondItem );
        }
    }

    for( item = aScreen->GetDrawItems(); item; item = secondItem )
    {
        secondItem = item->Next();

        if( item->GetEditFlags() & STRUCT_DELETED )
            RemoveFromScreen( item, aScreen );
    }

    if( itemList.GetCount() && aUndo )
        SaveCopyInUndoList( itemList, UR_DELETED, true );

    return itemList.GetCount() > 0;
}


bool SCH_EDIT_FRAME::AddMissingJunctions( SCH_SCREEN* aScreen )
{
     bool added = false;

    auto add_junction = [ & ]( const wxPoint& aPosition ) -> void
    {
        auto junction = new SCH_JUNCTION( aPosition );
        AddToScreen( junction, aScreen );
        BreakSegments( aPosition, false );
        added = true;
    };

    for( auto item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            auto line = static_cast<SCH_LINE*>( item );

             if( aScreen->IsJunctionNeeded( line->GetStartPoint(), true ) )
                add_junction( line->GetStartPoint() );

             if( aScreen->IsJunctionNeeded( line->GetEndPoint(), true ) )
                add_junction( line->GetEndPoint() );
        }
    }

    return added;
}


void SCH_EDIT_FRAME::NormalizeSchematicOnFirstLoad()
{
    BreakSegmentsOnJunctions();
    SchematicCleanUp();

    SCH_SHEET_LIST list( g_RootSheet );

    for( const auto& sheet : list )
        AddMissingJunctions( sheet.LastScreen() );
}


bool SCH_EDIT_FRAME::BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint,
                                   bool aAppend, SCH_LINE** aNewSegment,
                                   SCH_SCREEN* aScreen )
{
    if( !IsPointOnSegment( aSegment->GetStartPoint(), aSegment->GetEndPoint(), aPoint )
            || aSegment->IsEndPoint( aPoint ) )
        return false;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    SCH_LINE* newSegment = new SCH_LINE( *aSegment );

    newSegment->SetStartPoint( aPoint );
    AddToScreen( newSegment, aScreen );

    SaveCopyInUndoList( newSegment, UR_NEW, aAppend );
    SaveCopyInUndoList( aSegment, UR_CHANGED, true );

    RefreshItem( aSegment );
    aSegment->SetEndPoint( aPoint );

    if( aNewSegment )
        *aNewSegment = newSegment;

    return true;
}


bool SCH_EDIT_FRAME::BreakSegments( const wxPoint& aPoint, bool aAppend, SCH_SCREEN* aScreen )
{
    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool brokenSegments = false;

    for( SCH_ITEM* segment = aScreen->GetDrawItems(); segment; segment = segment->Next() )
    {
        if( ( segment->Type() != SCH_LINE_T ) || ( segment->GetLayer() == LAYER_NOTES ) )
            continue;

        brokenSegments |= BreakSegment( (SCH_LINE*) segment, aPoint,
                                        aAppend || brokenSegments, NULL, aScreen );
    }

    return brokenSegments;
}


bool SCH_EDIT_FRAME::BreakSegmentsOnJunctions( bool aAppend, SCH_SCREEN* aScreen )
{
    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool brokenSegments = false;

    for( SCH_ITEM* item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = ( SCH_JUNCTION* ) item;

            if( BreakSegments( junction->GetPosition(), brokenSegments || aAppend, aScreen ) )
                brokenSegments = true;
        }
        else
        {
            SCH_BUS_ENTRY_BASE* busEntry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( item );
            if( busEntry )
            {
                if( BreakSegments( busEntry->GetPosition(), brokenSegments || aAppend, aScreen ) )
                    brokenSegments = true;

                if( BreakSegments( busEntry->m_End(), brokenSegments || aAppend, aScreen ) )
                    brokenSegments = true;
            }
        }
    }

    return brokenSegments;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_ITEM* aJunction, bool aAppend )
{
    SCH_SCREEN* screen = GetScreen();
    PICKED_ITEMS_LIST itemList;

    auto remove_item = [ & ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        itemList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
        RemoveFromScreen( aItem );
    };

    remove_item( aJunction );

    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
    {
        SCH_LINE* firstLine = dynamic_cast<SCH_LINE*>( item );

        if( !firstLine || !firstLine->IsEndPoint( aJunction->GetPosition() )
                  || ( firstLine->GetEditFlags() & STRUCT_DELETED ) )
            continue;

        for( SCH_ITEM* secondItem = item->Next(); secondItem; secondItem = secondItem->Next() )
        {
            SCH_LINE* secondLine = dynamic_cast<SCH_LINE*>( secondItem );

            if( !secondLine || !secondLine->IsEndPoint( aJunction->GetPosition() )
                    || ( secondItem->GetEditFlags() & STRUCT_DELETED )
                    || !secondLine->IsParallel( firstLine ) )
                continue;


            // Remove identical lines
            if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
            {
                remove_item( secondItem );
                continue;
            }

            // Try to merge the remaining lines
            if( SCH_LINE* line = (SCH_LINE*) secondLine->MergeOverlap( firstLine ) )
            {
                remove_item( item );
                remove_item( secondItem );
                itemList.PushItem( ITEM_PICKER( line, UR_NEW ) );
                AddToScreen( line );
                break;
            }
        }
    }

    SaveCopyInUndoList( itemList, UR_DELETED, aAppend );

    SCH_ITEM* nextitem;
    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = nextitem )
    {
        nextitem = item->Next();

        if( item->GetEditFlags() & STRUCT_DELETED )
            RemoveFromScreen( item );
    }
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( const wxPoint& aPosition, bool aAppend, bool aFinal )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );
    bool broken_segments = false;

    AddToScreen( junction );
    broken_segments = BreakSegments( aPosition, aAppend );
    SaveCopyInUndoList( junction, UR_NEW, broken_segments || aAppend );

    if( aFinal )
    {
        TestDanglingEnds();
        OnModify();

        auto view = GetCanvas()->GetView();
        view->ClearPreview();
        view->ShowPreview( false );
        view->ClearHiddenFlags();
    }

    return junction;
}


