// SPDX-License-Identifier: LGPL-2.1-or-later
//
// LinuxCAD modern navigation style.
//
// Inherits CADNavigationStyle's well-understood mouse behavior (middle drag
// orbits, wheel zooms, click selects, click-drag on empty space rubberbands,
// right-click opens context menus) and layers touchpad-friendly extras on
// top so the same gestures work on a 3-button mouse and a Magic Trackpad:
//
//   * Alt + left drag  -> orbit  (single-finger orbit on a touchpad)
//   * Shift + left drag -> pan
//   * Shift + wheel     -> pan vertically (most touchpad two-finger scrolls
//                          come through as wheel events; with Shift this lets
//                          the user pan instead of zoom)
//   * Plain wheel / pinch -> zoom (CADNavigationStyle's default)
//
// The result is a single navigation style that "just works" whether the user
// drives the viewer with a mouse or a laptop touchpad.

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#endif

#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>

#include "Navigation/NavigationStyle.h"
#include "View3DInventorViewer.h"

using namespace Gui;

// ----------------------------------------------------------------------------------
// TRANSLATOR Gui::LinuxCadNavigationStyle

TYPESYSTEM_SOURCE(Gui::LinuxCadNavigationStyle, Gui::UserNavigationStyle)

LinuxCadNavigationStyle::LinuxCadNavigationStyle() = default;
LinuxCadNavigationStyle::~LinuxCadNavigationStyle() = default;

std::string LinuxCadNavigationStyle::userFriendlyName() const
{
    return "LinuxCAD modern (mouse + touchpad)";
}

const char* LinuxCadNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Click empty space to deselect, click an object to select");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Press middle mouse button, or hold Shift while dragging");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Hold Alt while dragging, or use middle button");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Scroll wheel / pinch on touchpad to zoom");
        default:
            return "No description";
    }
}

SbBool LinuxCadNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Defer to common housekeeping in NavigationStyle.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing()) {
        this->setViewing(false);
    }

    const SoType type(ev->getTypeId());
    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn = normalizePixelPos(pos);
    const SbVec2f prevnormalized = this->lastmouseposition;
    this->lastmouseposition = posn;

    SbBool processed = false;
    const ViewerMode curmode = this->currentmode;
    ViewerMode newmode = curmode;

    syncModifierKeys(ev);

    if (!viewer->isEditing()) {
        processed = handleEventInForeground(ev);
        if (processed) {
            return true;
        }
    }

    // Keyboard
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const auto event = static_cast<const SoKeyboardEvent*>(ev);
        processed = processKeyboardEvent(event);
    }

    // Mouse buttons - middle = orbit, left = select / drag, right = context
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto event = static_cast<const SoMouseButtonEvent*>(ev);
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                this->lockrecenter = true;
                this->button1down = press;
                if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                    newmode = NavigationStyle::SEEK_MODE;
                    this->seekToPoint(pos);
                    processed = true;
                }
                else if (press
                         && (this->currentmode == NavigationStyle::PANNING
                             || this->currentmode == NavigationStyle::ZOOMING)) {
                    newmode = NavigationStyle::DRAGGING;
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                    processed = true;
                }
                else if (viewer->isEditing()
                         && this->currentmode == NavigationStyle::SPINNING) {
                    processed = true;
                }
                else {
                    processed = processClickEvent(event);
                }
                break;

            case SoMouseButtonEvent::BUTTON2:
                this->lockrecenter = true;
                if (!press && (hasDragged || hasPanned || hasZoomed)) {
                    processed = true;
                }
                else if (!press && !viewer->isEditing()) {
                    if (this->currentmode != NavigationStyle::ZOOMING
                        && this->currentmode != NavigationStyle::PANNING
                        && this->currentmode != NavigationStyle::DRAGGING) {
                        if (this->isPopupMenuEnabled()) {
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }
                this->button2down = press;
                break;

            case SoMouseButtonEvent::BUTTON3:
                // Middle mouse button - the canonical CAD orbit gesture.
                if (press) {
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                }
                this->button3down = press;
                processed = true;
                break;

            case SoMouseButtonEvent::BUTTON4: // wheel up = zoom in
                if (press) {
                    if (this->shiftdown) {
                        // Shift + wheel = pan vertically (touchpad-friendly).
                        panCamera(viewer->getSoRenderManager()->getCamera(),
                                  vp.getViewportAspectRatio(),
                                  this->panningplane,
                                  SbVec2f(posn[0], posn[1] + 0.05f),
                                  posn);
                    }
                    else {
                        doZoom(viewer->getSoRenderManager()->getCamera(), 1, posn);
                    }
                    processed = true;
                }
                break;

            case SoMouseButtonEvent::BUTTON5: // wheel down = zoom out
                if (press) {
                    if (this->shiftdown) {
                        panCamera(viewer->getSoRenderManager()->getCamera(),
                                  vp.getViewportAspectRatio(),
                                  this->panningplane,
                                  SbVec2f(posn[0], posn[1] - 0.05f),
                                  posn);
                    }
                    else {
                        doZoom(viewer->getSoRenderManager()->getCamera(), -1, posn);
                    }
                    processed = true;
                }
                break;

            default:
                break;
        }
    }

    // Mouse motion - decides what to do based on current mode.
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const auto event = static_cast<const SoLocation2Event*>(ev);
        if (this->currentmode == NavigationStyle::ZOOMING) {
            this->zoomByCursor(posn, prevnormalized);
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::PANNING) {
            if (!blockPan) {
                float ratio = vp.getViewportAspectRatio();
                panCamera(viewer->getSoRenderManager()->getCamera(),
                          ratio,
                          this->panningplane,
                          posn,
                          prevnormalized);
            }
            blockPan = false;
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::DRAGGING) {
            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = true;
        }
    }

    // Mode resolution. Modifier keys override mouse-button-driven mode so
    // touchpad users (no middle button) can still pan / orbit with Shift /
    // Alt held, and middle-button mouse users still get the canonical CAD
    // orbit-on-middle-drag.
    enum
    {
        BUTTON1DOWN = 1 << 0,
        BUTTON2DOWN = 1 << 1,
        BUTTON3DOWN = 1 << 2,
        CTRLDOWN    = 1 << 3,
        SHIFTDOWN   = 1 << 4,
        ALTDOWN     = 1 << 5
    };
    const unsigned int combo = (this->button1down ? BUTTON1DOWN : 0)
        | (this->button2down ? BUTTON2DOWN : 0) | (this->button3down ? BUTTON3DOWN : 0)
        | (this->ctrldown ? CTRLDOWN : 0) | (this->shiftdown ? SHIFTDOWN : 0)
        | (this->altdown ? ALTDOWN : 0);

    if (combo == 0) {
        if (curmode == NavigationStyle::SPINNING) {
            // let it keep spinning
        }
        else {
            newmode = NavigationStyle::IDLE;
        }
    }
    else if (combo & BUTTON3DOWN) {
        // Middle-drag orbits, +Shift pans, +Ctrl zooms.
        if ((combo & SHIFTDOWN) && !(combo & CTRLDOWN)) {
            newmode = NavigationStyle::PANNING;
            if (currentmode != NavigationStyle::PANNING) {
                blockPan = true;
                saveCursorPosition(ev);
            }
        }
        else if (combo & CTRLDOWN) {
            newmode = NavigationStyle::ZOOMING;
        }
        else {
            if (newmode != NavigationStyle::DRAGGING) {
                saveCursorPosition(ev);
            }
            newmode = NavigationStyle::DRAGGING;
        }
    }
    else if (combo & BUTTON1DOWN) {
        if (combo & ALTDOWN) {
            // Alt + left drag = orbit (great for single-finger touchpad use)
            if (newmode != NavigationStyle::DRAGGING) {
                saveCursorPosition(ev);
            }
            newmode = NavigationStyle::DRAGGING;
        }
        else if (combo & SHIFTDOWN) {
            newmode = NavigationStyle::PANNING;
            if (currentmode != NavigationStyle::PANNING) {
                blockPan = true;
            }
        }
        else if (combo & CTRLDOWN) {
            newmode = NavigationStyle::ZOOMING;
        }
        else {
            // Plain left drag - selection / rubberband (NavigationStyle handles it).
            if (curmode == NavigationStyle::SPINNING) {
                newmode = NavigationStyle::IDLE;
            }
            else {
                newmode = NavigationStyle::SELECTION;
            }
        }
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    if (processed) {
        return true;
    }

    return inherited::processSoEvent(ev);
}
