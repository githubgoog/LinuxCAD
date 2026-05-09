// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWizardPage>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Navigation/NavigationStyle.h>

#include "AI/ConfigureAiDialog.h"
#include "AI/Provider.h"
#include "FirstRunWizard.h"
#include "LinuxCadShell.h"
#include "Theme.h"

#include <QWizard>

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr const char* kConsentPromptedKey = "LinuxCAD/AI/ConsentPromptedV1";

int schemaIndexByName(const std::string& name)
{
    const auto names = Base::UnitsApi::getNames();
    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void applyUnits(bool imperialInches)
{
    ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Units");

    const int idx = imperialInches ? schemaIndexByName("ImperialDecimal")
                                   : schemaIndexByName("MmMin");

    hGrpu->SetInt("UserSchema", idx);
    Base::UnitsApi::setSchema(static_cast<std::size_t>(idx));

    Gui::MainWindow* mw = Gui::getMainWindow();
    if (mw != nullptr) {
        mw->setUserSchema(idx);
    }
}

void applyNavigation(bool /*unifiedTouchPad*/)
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    const char* ascii = LinuxCadNavigationStyle::getClassTypeId().getName();
    hGrp->SetASCII("NavigationStyle", ascii);
}

class FirstRunInstallerWizard final : public QWizard
{
public:
    explicit FirstRunInstallerWizard(QWidget* parent = nullptr)
        : QWizard(parent)
    {
        setWizardStyle(QWizard::ModernStyle);
        setWindowTitle(tr("LinuxCAD setup"));

        themeDark_    = nullptr;
        themeLight_      = nullptr;
        unitsMetric_     = nullptr;
        unitsImperial_   = nullptr;
        aiMock_          = nullptr;
        aiOpenAi_        = nullptr;
        aiAnthropic_     = nullptr;

        auto* pgTheme           = buildThemePage();
        auto* pgUnits           = buildUnitsPage();
        auto* pgAi              = buildAiPage();

        addPage(pgTheme);
        addPage(pgUnits);
        addPage(pgAi);

        resize(560, 420);
    }

private:
    QWizardPage* buildThemePage()
    {
        auto* page = new QWizardPage(this);
        page->setTitle(tr("Appearance"));
        page->setSubTitle(tr("Pick a workspace theme."));
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(new QLabel(tr("LinuxCAD adapts toolbar and chrome colors to your preference."),
                                  page));

        themeDark_  = new QRadioButton(tr("Dark (default)"), page);
        themeLight_ = new QRadioButton(tr("Light"), page);

        themeDark_->setChecked(true);

        auto* grpTheme = new QButtonGroup(page);
        grpTheme->addButton(themeDark_);
        grpTheme->addButton(themeLight_);

        lay->addWidget(themeDark_);
        lay->addWidget(themeLight_);
        lay->addStretch();
        return page;
    }

    QWizardPage* buildUnitsPage()
    {
        auto* page = new QWizardPage(this);
        page->setTitle(tr("Units"));
        page->setSubTitle(tr("Default unit system for new work."));
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(new QLabel(tr("You can switch again in Preferences at any time."), page));

        unitsMetric_ = new QRadioButton(tr("Millimeters / Metric CNC (recommended)"), page);
        unitsImperial_
            = new QRadioButton(tr("Inches — Imperial decimal"), page);
        unitsMetric_->setChecked(true);

        auto* grpUnits = new QButtonGroup(page);
        grpUnits->addButton(unitsMetric_);
        grpUnits->addButton(unitsImperial_);

        lay->addWidget(unitsMetric_);
        lay->addWidget(unitsImperial_);
        lay->addStretch();
        return page;
    }

    QWizardPage* buildAiPage()
    {
        auto* page = new QWizardPage(this);
        page->setTitle(tr("AI assistant"));
        page->setSubTitle(tr("Optional modeling suggestions (bring-your-own-key for cloud APIs)."));

        auto* lay = new QVBoxLayout(page);
        lay->addWidget(new QLabel(tr("Choose who powers inline suggestions."), page));

        aiMock_      = new QRadioButton(tr("Mock — offline canned responses (recommended for trials)"),
                                   page);
        aiOpenAi_    = new QRadioButton(tr("OpenAI-compatible API"), page);
        aiAnthropic_ = new QRadioButton(tr("Anthropic Claude"), page);
        aiMock_->setChecked(true);

        aiEnableCb_
            = new QCheckBox(tr("Enable AI suggestions now (you change this anytime)"), page);
        aiEnableCb_->setChecked(true);

        auto* grpAi = new QButtonGroup(page);
        grpAi->addButton(aiMock_);
        grpAi->addButton(aiOpenAi_);
        grpAi->addButton(aiAnthropic_);

        lay->addWidget(aiMock_);
        lay->addWidget(aiOpenAi_);
        lay->addWidget(aiAnthropic_);
        lay->addSpacing(12);
        lay->addWidget(aiEnableCb_);

        lay->addStretch();

        lay->addWidget(new QLabel(tr("You can rerun this wizard anytime from "
                                      "LinuxCAD logo → Re-run setup wizard."),
                                  page));

        return page;
    }

    void accept() override
    {
        // --- Theme ---
        Theme::Variant tv = Theme::Variant::Dark;
        if (themeLight_ != nullptr && themeLight_->isChecked()) {
            tv = Theme::Variant::Light;
        }

        Shell* shell = Shell::instance();
        if (shell != nullptr && shell->theme() != nullptr) {
            shell->theme()->applyVariant(tv);
        }
        else if (shell != nullptr) {
            guiAppApplyThemeFallback(tv);
        }

        // --- Units ---
        applyUnits(unitsImperial_ != nullptr && unitsImperial_->isChecked());

        // --- Navigation: always LinuxCAD style; choice is no longer offered. ---
        applyNavigation(true);

        // --- AI + consent bookkeeping ---
        QSettings st;

        QString providerKind = QStringLiteral("mock");
        if (aiAnthropic_ != nullptr && aiAnthropic_->isChecked()) {
            providerKind = QStringLiteral("anthropic");
        }
        else if (aiOpenAi_ != nullptr && aiOpenAi_->isChecked()) {
            providerKind = QStringLiteral("openai");
        }

        st.setValue(QString::fromLatin1(Provider::kSettingProvider), providerKind);
        if (providerKind.compare(QStringLiteral("mock"), Qt::CaseInsensitive) == 0) {
            st.setValue(QString::fromLatin1(Provider::kSettingApiKey), QString());
            st.setValue(QString::fromLatin1(Provider::kSettingModel), QString());
        }

        const bool aiOn = aiEnableCb_ != nullptr && aiEnableCb_->isChecked();
        st.setValue(QString::fromLatin1(Provider::kSettingConsentGiven), aiOn);
        st.setValue(QString::fromLatin1(Provider::kSettingEnabled), aiOn);

        // Skip the standalone consent dialog - user already opted here.
        st.setValue(QLatin1String(kConsentPromptedKey), true);

        st.sync();

        if (providerKind.compare(QStringLiteral("mock"), Qt::CaseInsensitive) != 0) {
            QWidget* dlgParent = Gui::getMainWindow();
            if (shell != nullptr && shell->mainWindow() != nullptr) {
                dlgParent = shell->mainWindow();
            }
            ConfigureAiDialog::run(dlgParent);
        }

        if (shell != nullptr) {
            shell->reloadAiProvider();
        }

        QWizard::accept();
    }

    void guiAppApplyThemeFallback(Theme::Variant v)
    {
        Q_UNUSED(v);
        if (Gui::Application::Instance != nullptr) {
            try {
                Gui::Application::Instance->reloadStyleSheet();
            }
            catch (...) {}
        }
    }

    QRadioButton* themeDark_     = nullptr;
    QRadioButton* themeLight_    = nullptr;
    QRadioButton* unitsMetric_   = nullptr;
    QRadioButton* unitsImperial_ = nullptr;
    QRadioButton* aiMock_        = nullptr;
    QRadioButton* aiOpenAi_      = nullptr;
    QRadioButton* aiAnthropic_ = nullptr;

    QCheckBox* aiEnableCb_ = nullptr;
};

void promptIfNeededImpl(QWidget* parent)
{
    QSettings s;
    if (s.value(QLatin1String(FirstRunWizard::kCompletedKey), false).toBool()) {
        return;
    }

    FirstRunInstallerWizard wiz(parent);
    if (wiz.exec() == QDialog::Accepted) {
        s.setValue(QLatin1String(FirstRunWizard::kCompletedKey), true);
    }
}

void runAgainImpl(QWidget* parent)
{
    QSettings s;
    s.remove(QLatin1String(FirstRunWizard::kCompletedKey));

    FirstRunInstallerWizard wiz(parent);
    if (wiz.exec() == QDialog::Accepted) {
        s.setValue(QLatin1String(FirstRunWizard::kCompletedKey), true);
    }
}

} // namespace

void FirstRunWizard::promptIfNeeded(QWidget* parent)
{
    promptIfNeededImpl(parent);
}

void FirstRunWizard::runAgain(QWidget* parent)
{
    runAgainImpl(parent);
}

void FirstRunWizard::applySilentDefaults()
{
    QSettings s;
    if (s.value(QLatin1String(FirstRunWizard::kCompletedKey), false).toBool()) {
        return;
    }

    // Theme: dark by default.
    Shell* shell = Shell::instance();
    if (shell != nullptr && shell->theme() != nullptr) {
        shell->theme()->applyVariant(Theme::Variant::Dark);
    }

    // Units: metric.
    applyUnits(/*imperialInches=*/false);

    // Navigation: LinuxCAD unified mouse + touchpad.
    applyNavigation(/*unused*/ true);

    // AI: mock provider, disabled, consent recorded as prompted (silent).
    s.setValue(QString::fromLatin1(Provider::kSettingProvider),
               QStringLiteral("mock"));
    s.setValue(QString::fromLatin1(Provider::kSettingApiKey), QString());
    s.setValue(QString::fromLatin1(Provider::kSettingModel), QString());
    s.setValue(QString::fromLatin1(Provider::kSettingConsentGiven), false);
    s.setValue(QString::fromLatin1(Provider::kSettingEnabled), false);
    s.setValue(QLatin1String(kConsentPromptedKey), true);

    s.setValue(QLatin1String(FirstRunWizard::kCompletedKey), true);
    s.sync();
}

} // namespace LinuxCAD
} // namespace Gui
