/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2019 KiCad Developers, see change_log.txt for contributors.
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

#ifndef DRC_H
#define DRC_H

#include <class_board.h>
#include <class_track.h>
#include <class_marker_pcb.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <memory>
#include <vector>
#include <tools/pcb_tool_base.h>

#define OK_DRC  0
#define BAD_DRC 1



/// DRC error codes:
enum PCB_DRC_CODE {
    DRCE_FIRST = 1,
    DRCE_UNCONNECTED_ITEMS = DRCE_FIRST,    ///< items are unconnected
    DRCE_TRACK_NEAR_THROUGH_HOLE,           ///< thru hole is too close to track
    DRCE_TRACK_NEAR_PAD,                    ///< pad too close to track
    DRCE_TRACK_NEAR_VIA,                    ///< track too close to via
    DRCE_VIA_NEAR_VIA,                      ///< via too close to via
    DRCE_VIA_NEAR_TRACK,                    ///< via too close to track
    DRCE_TRACK_ENDS,                        ///< track ends are too close
    DRCE_TRACK_SEGMENTS_TOO_CLOSE,          ///< 2 parallel track segments too close: segm ends between segref ends
    DRCE_TRACKS_CROSSING,                   ///< tracks are crossing
    DRCE_PAD_NEAR_PAD1,                     ///< pad too close to pad
    DRCE_VIA_HOLE_BIGGER,                   ///< via's hole is bigger than its diameter
    DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR,    ///< micro via's layer pair incorrect (layers must be adjacent)
    DRCE_ZONES_INTERSECT,                   ///< copper area outlines intersect
    DRCE_ZONES_TOO_CLOSE,                   ///< copper area outlines are too close
    DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE,   ///< copper area has a net but no pads in nets, which is suspicious
    DRCE_HOLE_NEAR_PAD,                     ///< hole too close to pad
    DRCE_HOLE_NEAR_TRACK,                   ///< hole too close to track
    DRCE_TOO_SMALL_TRACK_WIDTH,             ///< Too small track width
    DRCE_TOO_SMALL_VIA,                     ///< Too small via size
    DRCE_TOO_SMALL_MICROVIA,                ///< Too small micro via size
    DRCE_TOO_SMALL_VIA_DRILL,               ///< Too small via drill
    DRCE_TOO_SMALL_MICROVIA_DRILL,          ///< Too small micro via drill
    DRCE_NETCLASS_TRACKWIDTH,               ///< netclass has TrackWidth < board.m_designSettings->m_TrackMinWidth
    DRCE_NETCLASS_CLEARANCE,                ///< netclass has Clearance < board.m_designSettings->m_TrackClearance
    DRCE_NETCLASS_VIASIZE,                  ///< netclass has ViaSize < board.m_designSettings->m_ViasMinSize
    DRCE_NETCLASS_VIADRILLSIZE,             ///< netclass has ViaDrillSize < board.m_designSettings->m_ViasMinDrill
    DRCE_NETCLASS_uVIASIZE,                 ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinSize
    DRCE_NETCLASS_uVIADRILLSIZE,            ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinDrill
    DRCE_VIA_INSIDE_KEEPOUT,                ///< Via in inside a keepout area
    DRCE_TRACK_INSIDE_KEEPOUT,              ///< Track in inside a keepout area
    DRCE_PAD_INSIDE_KEEPOUT,                ///< Pad in inside a keepout area
    DRCE_TRACK_NEAR_COPPER,                 ///< track & copper graphic collide or are too close
    DRCE_VIA_NEAR_COPPER,                   ///< via and copper graphic collide or are too close
    DRCE_PAD_NEAR_COPPER,                   ///< pad and copper graphic collide or are too close
    DRCE_TRACK_NEAR_ZONE,                   ///< track & zone collide or are too close together
    DRCE_OVERLAPPING_FOOTPRINTS,            ///< footprint courtyards overlap
    DRCE_MISSING_COURTYARD_IN_FOOTPRINT,    ///< footprint has no courtyard defined
    DRCE_MALFORMED_COURTYARD_IN_FOOTPRINT,  ///< footprint has a courtyard but malformed
                                                    ///< (not convertible to a closed polygon with holes)
    DRCE_MICRO_VIA_NOT_ALLOWED,             ///< micro vias are not allowed
    DRCE_BURIED_VIA_NOT_ALLOWED,            ///< buried vias are not allowed
    DRCE_DISABLED_LAYER_ITEM,               ///< item on a disabled layer
    DRCE_DRILLED_HOLES_TOO_CLOSE,           ///< overlapping drilled holes break drill bits
    DRCE_TRACK_NEAR_EDGE,                   ///< track too close to board edge
    DRCE_INVALID_OUTLINE,                   ///< invalid board outline
    DRCE_MISSING_FOOTPRINT,                 ///< footprint not found for netlist item
    DRCE_DUPLICATE_FOOTPRINT,               ///< more than one footprints found for netlist item
    DRCE_EXTRA_FOOTPRINT,                   ///< netlist item not found for footprint

    DRCE_SHORT,
    DRCE_REDUNDANT_VIA,
    DRCE_DUPLICATE_TRACK,
    DRCE_MERGE_TRACKS,
    DRCE_DANGLING_TRACK,
    DRCE_DANGLING_VIA,
    DRCE_ZERO_LENGTH_TRACK,
    DRCE_TRACK_IN_PAD,

    DRCE_UNRESOLVED_VARIABLE,

    DRCE_LAST = DRCE_UNRESOLVED_VARIABLE
};


class PCB_EDIT_FRAME;
class DIALOG_DRC;
class BOARD_ITEM;
class BOARD;
class D_PAD;
class ZONE_CONTAINER;
class TRACK;
class MARKER_PCB;
class DRC_ITEM;
class NETCLASS;
class EDA_TEXT;
class DRAWSEGMENT;
class NETLIST;
class wxWindow;
class wxString;
class wxTextCtrl;


/**
 * Design Rule Checker object that performs all the DRC tests.  The output of
 * the checking goes to the BOARD file in the form of two MARKER lists.  Those
 * two lists are displayable in the drc dialog box.  And they can optionally
 * be sent to a text file on disk.
 * This class is given access to the windows and the BOARD
 * that it needs via its constructor or public access functions.
 */
class DRC : public PCB_TOOL_BASE
{
    friend class DIALOG_DRC;

public:
    DRC();
    ~DRC();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

private:

    //  protected or private functions() are lowercase first character.
    bool     m_doPad2PadTest;           // enable pad to pad clearance tests
    bool     m_doUnconnectedTest;       // enable unconnected tests
    bool     m_doZonesTest;             // enable zone to items clearance tests
    bool     m_doKeepoutTest;           // enable keepout areas to items clearance tests
    bool     m_refillZones;             // refill zones if requested (by user).
    bool     m_reportAllTrackErrors;    // Report all tracks errors (or only 4 first errors)
    bool     m_testFootprints;          // Test footprints against schematic

    /* In DRC functions, many calculations are using coordinates relative
     * to the position of the segment under test (segm to segm DRC, segm to pad DRC
     * Next variables store coordinates relative to the start point of this segment
     */
    wxPoint  m_padToTestPos;            // Position of the pad for segm-to-pad and pad-to-pad
    wxPoint  m_segmEnd;                 // End point of the reference segment (start = (0, 0) )

    /* Some functions are comparing the ref segm to pads or others segments using
     * coordinates relative to the ref segment considered as the X axis
     * so we store the ref segment length (the end point relative to these axis)
     * and the segment orientation (used to rotate other coordinates)
     */
    double   m_segmAngle;               // Ref segm orientation in 0.1 degree
    int      m_segmLength;              // length of the reference segment

    /* variables used in checkLine to test DRC segm to segm:
     * define the area relative to the ref segment that does not contains any other segment
     */
    int      m_xcliplo;
    int      m_ycliplo;
    int      m_xcliphi;
    int      m_ycliphi;

    PCB_EDIT_FRAME*        m_pcbEditorFrame;   // The pcb frame editor which owns the board
    BOARD*                 m_pcb;
    SHAPE_POLY_SET         m_board_outlines;   // The board outline including cutouts
    DIALOG_DRC*    m_drcDialog;

    std::vector<DRC_ITEM*> m_unconnected;      // list of unconnected pads
    std::vector<DRC_ITEM*> m_footprints;       // list of footprint warnings
    bool                   m_drcRun;
    bool                   m_footprintsTested;

    ///> Sets up handlers for various events.
    void setTransitions() override;

    /**
     * Update needed pointers from the one pointer which is known not to change.
     */
    void updatePointers();

    EDA_UNITS userUnits() const { return m_pcbEditorFrame->GetUserUnits(); }

    /**
     * Adds a DRC marker to the PCB through the COMMIT mechanism.
     */
    void addMarkerToPcb( MARKER_PCB* aMarker );

    /**
     * Fetches a reasonable point for marking a violoation between two non-point objects.
     */
    wxPoint getLocation( TRACK* aTrack, ZONE_CONTAINER* aConflictZone ) const;
    wxPoint getLocation( TRACK* aTrack, BOARD_ITEM* aConflitItem, const SEG& aConflictSeg ) const;

    //-----<categorical group tests>-----------------------------------------

    /**
     * Go through each NETCLASS and verifies that its clearance, via size, track width, and
     * track clearance are larger than those in board.m_designSettings.
     * This is necessary because the actual DRC checks are run against the NETCLASS
     * limits, so in order enforce global limits, we first check the NETCLASSes against
     * the global limits.
     * @return bool - true if succes, else false but only after
     *  reporting _all_ NETCLASS violations.
     */
    bool testNetClasses();

    /**
     * Perform the DRC on all tracks.
     *
     * This test can take a while, a progress bar can be displayed
     * @param aActiveWindow = the active window ued as parent for the progress bar
     * @param aShowProgressBar = true to show a progress bar
     * (Note: it is shown only if there are many tracks)
     */
    void testTracks( wxWindow * aActiveWindow, bool aShowProgressBar );

    void testPad2Pad();

    void testDrilledHoles();

    void testUnconnected();

    void testZones();

    void testKeepoutAreas();

    // aTextItem is type BOARD_ITEM* to accept either TEXTE_PCB or TEXTE_MODULE
    void testCopperTextItem( BOARD_ITEM* aTextItem );

    void testCopperDrawItem( DRAWSEGMENT* aDrawing );

    void testCopperTextAndGraphics();

    // Tests for items placed on disabled layers (causing false connections).
    void testDisabledLayers();

    // Test for any unresolved text variable references
    void testTextVars();

    /**
     * Test that the board outline is contiguous and composed of valid elements
     */
    void testOutline();

    //-----<single "item" tests>-----------------------------------------

    bool doNetClass( const std::shared_ptr<NETCLASS>& aNetClass, wxString& msg );

    /**
     * Test the clearance between aRefPad and other pads.
     *
     * The pad list must be sorted by x coordinate.
     *
     * @param aRefPad is the pad to test
     * @param aStart is the first pad of the list to test against aRefPad
     * @param aEnd is the end of the list and is not included
     * @param x_limit is used to stop the test
     * (i.e. when the current pad pos X in list exceeds this limit, because the list
     * is sorted by X coordinate)
     */
    bool doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    /**
     * Test the current segment.
     *
     * @param aRefSeg The segment to test
     * @param aStartIt the iterator to the first track to test
     * @param aEndIt the marker for the iterator end
     * @param aTestZones true if should do copper zones test. This can be very time consumming
     * @return bool - true if no problems, else false and m_currentMarker is
     *          filled in with the problem information.
     */
    void doTrackDrc( TRACK* aRefSeg, TRACKS::iterator aStartIt, TRACKS::iterator aEndIt,
                     bool aTestZones );

    /**
     * Test for footprint courtyard overlaps.
     */
    void doOverlappingCourtyardsDrc();

    //-----<single tests>----------------------------------------------

    /**
     * @param aRefPad The reference pad to check
     * @param aPad Another pad to check against
     * @return bool - true if clearance between aRefPad and aPad is >= dist_min, else false
     */
    bool checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad );


    /**
     * Check the distance from a pad to segment.  This function uses several
     * instance variable not passed in:
     *      m_segmLength = length of the segment being tested
     *      m_segmAngle  = angle of the segment with the X axis;
     *      m_segmEnd    = end coordinate of the segment
     *      m_padToTestPos = position of pad relative to the origin of segment
     * @param aPad Is the pad involved in the check
     * @param aSegmentWidth width of the segment to test
     * @param aMinDist Is the minimum clearance needed
     *
     * @return true distance >= dist_min,
     *         false if distance < dist_min
     */
    bool checkClearanceSegmToPad( const D_PAD* aPad, int aSegmentWidth, int aMinDist );


    /**
     * Check the distance from a point to a segment.
     *
     * The segment is expected starting at 0,0, and on the X axis
     * (used to test DRC between a segment and a round pad, via or round end of a track
     * @param aCentre The coordinate of the circle's center
     * @param aRadius A "keep out" radius centered over the circle
     * @param aLength The length of the segment (i.e. coordinate of end, because it is on
     *                the X axis)
     * @return bool - true if distance >= radius, else
     *                false when distance < aRadius
     */
    static bool checkMarginToCircle( wxPoint aCentre, int aRadius, int aLength );


    /**
     * Function checkLine
     * (helper function used in drc calculations to see if one track is in contact with
     *  another track).
     * Test if a line intersects a bounding box (a rectangle)
     * The rectangle is defined by m_xcliplo, m_ycliplo and m_xcliphi, m_ycliphi
     * return true if the line from aSegStart to aSegEnd is outside the bounding box
     */
    bool checkLine( wxPoint aSegStart, wxPoint aSegEnd );

    //-----</single tests>---------------------------------------------

public:
    /**
     * Tests whether distance between zones complies with the DRC rules.
     *
     * @return Errors count
     */
    int TestZoneToZoneOutlines();

    /**
     * Test the board footprints against a netlist.  Will report DRCE_MISSING_FOOTPRINT,
     * DRCE_DUPLICATE_FOOTPRINT and DRCE_EXTRA_FOOTPRINT errors in aDRCList.
     */
    static void TestFootprints( NETLIST& aNetlist, BOARD* aPCB, EDA_UNITS aUnits,
                                std::vector<DRC_ITEM*>& aDRCList );

    /**
     * Open a dialog and prompts the user, then if a test run button is
     * clicked, runs the test(s) and creates the MARKERS.  The dialog is only
     * created if it is not already in existence.
     *
     * @param aParent is the parent window for wxWidgets. Usually the PCB editor frame
     * but can be another dialog
     * if aParent == NULL (default), the parent will be the PCB editor frame
     * and the dialog will be not modal (just float on parent
     * if aParent is specified, the dialog will be modal.
     * The modal mode is mandatory if the dialog is created from another dialog, not
     * from the PCB editor frame
     */
    void ShowDRCDialog( wxWindow* aParent );

    int ShowDRCDialog( const TOOL_EVENT& aEvent );

    /**
     * Check to see if the DRC dialog is currently shown
     *
     * @return true if the dialog is shown
     */
    bool IsDRCDialogShown();

    /**
     * Deletes this ui dialog box and zeros out its pointer to remember
     * the state of the dialog's existence.
     *
     * @param aReason Indication of which button was clicked to cause the destruction.
     * if aReason == wxID_OK, design parameters values which can be entered from the dialog
     * will bbe saved in design parameters list
     */
    void DestroyDRCDialog( int aReason );

    /**
     * Run all the tests specified with a previous call to
     * SetSettings()
     * @param aMessages = a wxTextControl where to display some activity messages. Can be NULL
     */
    void RunTests( wxTextCtrl* aMessages = NULL );
};


#endif  // DRC_H
