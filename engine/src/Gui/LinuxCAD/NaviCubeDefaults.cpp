// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QSettings>
#include <QString>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "NaviCubeDefaults.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr const char* kFlagKey = "LinuxCAD/NaviCubeDefaultsAppliedV1";

ParameterGrp::handle naviCubeGroup()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/NaviCube");
}

ParameterGrp::handle viewGroup()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
}

void writeDefaults()
{
    auto nc = naviCubeGroup();
    if (!nc) {
        return;
    }

    // Geometry tweaks for a softer, more modern feel.
    nc->SetInt("CubeSize", 130);
    nc->SetFloat("ChamferSize", 0.18f);          // maximum rounding (0.05 - 0.18)
    nc->SetBool("NaviRotateToNearest", true);
    nc->SetInt("NaviStepByTurn", 12);            // smoother snap animation
    nc->SetFloat("BorderWidth", 0.6f);           // subtler edges
    nc->SetInt("InactiveOpacity", 55);           // 0-100, integer percent

    // Colors are stored as packed RGBA uint32 (R high byte). LinuxCAD's
    // accent for highlight; soft slate base; white-ish emphase.
    constexpr unsigned long kBaseSlate    = 0xC8D0DCFFu;  // soft slate face
    constexpr unsigned long kEmphaseWhite = 0xFFFFFFFFu;  // crisp emphase
    constexpr unsigned long kHiliteAccent = 0x418FDEFFu;  // LinuxCAD blue
    nc->SetUnsigned("BaseColor",    kBaseSlate);
    nc->SetUnsigned("EmphaseColor", kEmphaseWhite);
    nc->SetUnsigned("HiliteColor",  kHiliteAccent);

    // Coordinate system on, font weight medium-bold for readability.
    nc->SetBool("ShowCS", true);
    nc->SetInt("FontWeight", 57);   // QFont::Medium-ish
    nc->SetFloat("FontZoom", 0.32f);

    // Keep rotation animations on, and ensure NaviCube is shown by default.
    if (auto v = viewGroup()) {
        v->SetBool("ShowNaviCube", true);
        v->SetBool("UseNavigationAnimations", true);
    }
}

} // namespace

void NaviCubeDefaults::applyOnce()
{
    QSettings s;
    if (s.value(QString::fromLatin1(kFlagKey), false).toBool()) {
        return;
    }
    Base::Console().log("LinuxCAD: applying NaviCube polish defaults\n");
    writeDefaults();
    s.setValue(QString::fromLatin1(kFlagKey), true);
}

void NaviCubeDefaults::applyForce()
{
    Base::Console().log("LinuxCAD: forcing NaviCube polish defaults\n");
    writeDefaults();
    QSettings s;
    s.setValue(QString::fromLatin1(kFlagKey), true);
}

} // namespace LinuxCAD
} // namespace Gui
