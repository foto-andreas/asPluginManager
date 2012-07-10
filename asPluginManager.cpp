#include "asPluginManager.h"

#include <QObject>
#include <QAbstractButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>

#include <QFile>
#include <QDir>

#include <QList>
#include <QHash>

#include <QTimer>

#include <QDomDocument>

#include <PluginHub.h>
#include <PluginImageSettings.h>
#include <PluginOptionList.h>
#include <B5Plugin.h>

#include "TargetVersion.h"

#include "ToolData.h"

extern "C" BIBBLE_API BaseB5Plugin *b5plugin() { return new asPluginManager; }

bool asPluginManager::init(PluginHub *hub, int id, int groupId, const QString&) {

    qDebug() << "asPluginManager::init";

    m_hub = hub;
    m_pluginId = id;
    m_groupId = groupId;

    // configuration file abstraction with a configuration mapper
    QString configDir = m_hub->property("pluginStorageHome").toString() + "/asPluginManager";
    QDir qdir;
    qdir.mkdir(configDir);
    QString configPath = configDir + "/asPluginManager.conf";
    m_config = new ConfigurationMapper(configPath);
    if (m_config == NULL) {
        QMessageBox::information(NULL,"asPluginManager",tr("Configuration file problem with file:") + "<br/>" + configPath);
        return false;
    }

    return true;
}

void asPluginManager::checkForUpdates(QString id, int sdkVersion, QString installedVersion) {
    // check for updates
    if (m_config->checkForUpdates()) {
        WebInfos *webInfos = new WebInfos(id, QString("%1").arg(sdkVersion), installedVersion);
        connect(webInfos,
                SIGNAL(ready()),
                SLOT(webInfosReady()));
        qDebug() << "asPluginManager: fetching update infos for" << id;
        webInfos->fetch();
    }
}

#define min(a,b) (a>b ? b : a)

void asPluginManager::toolWidgetCreated(QWidget *uiWidget) {

    qDebug() << "asPluginManager::toolWidgetCreated";

    QWidget *contents = uiWidget->findChild<QWidget*>("contents");
    QGridLayout *layout = (QGridLayout*)contents->layout();

    layout->addWidget(new QLabel(tr("Loaded")), 0, 0, Qt::AlignLeft);
    layout->addWidget(new QLabel(tr("Version")), 0, 1, Qt::AlignLeft);
    layout->addWidget(new QLabel(tr("Enabled")), 0, 2, Qt::AlignLeft);

    int height = 0;

    connect(m_hub, SIGNAL(pluginDataComplete(const QString &, const PluginData *)), SLOT(handleDataComplete(const QString &, const PluginData *)));
    connect(m_hub, SIGNAL(pluginDataInvalid(const QString &)), SLOT(handleDataInvalid(const QString &)));

    // search the plugins and build the user interface
    QString dir = m_hub->property("pluginStorageHome").toString().append("/../Plugins");
    m_dir = new QDir(dir);
    m_dir->setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QStringList entries = m_dir->entryList();
    for (int i=0; i<entries.length(); i++) {

        LayoutData *layoutData = new LayoutData();

        QString name = entries[i];

        QString infoName = name;
        QString infoId = name;
        QString infoVersion = "";
        int infoMajor = 0;
        int infoMinor = 0;
        int infoFix = 0;
        int infoSDK = 0;
        QString infoFileName = dir + "/" + name + "/" + "Info.afpxml";
        try {
            QDomDocument infoDoc;
            QFile *infoFile = new QFile(infoFileName);
            infoDoc.setContent(infoFile);
            infoFile->close();
            delete infoFile;
            QDomElement propertyScribe = infoDoc.documentElement();
            QDomElement pluginFileData = propertyScribe.elementsByTagName("PluginFileData").at(0).toElement();
            infoName = pluginFileData.attribute("name");
            infoId = pluginFileData.attribute("identifier");
            infoMajor = pluginFileData.attribute("majorVersion").toInt();
            infoMinor = pluginFileData.attribute("minorVersion").toInt();
            infoFix = pluginFileData.attribute("bugfixVersion").toInt();
            infoSDK = pluginFileData.attribute("sdkVersion").toInt();
            infoVersion = QString("%1.%2.%3").arg(infoMajor).arg(infoMinor).arg(infoFix);
            qDebug() << "asPluginManager: found plugin dir" << name
                     << "with name" << infoName << "with id" << infoId
                     << "in version" << infoVersion << "for sdk version" << infoSDK;
        } catch (exception e) {
            qDebug() << "asPluginManager:" << infoFileName << "has unknown structure.";
        }

        name.remove(QRegExp("\\.afplugin(\\.off)*$"));

        layoutData->name = name;
        layoutData->infoId = infoId;
        if (entries[i].endsWith("afplugin")) {
            layoutData->loaded = true;
        }

        QString internalName = m_config->getString(name,NULL);
        if (internalName == NULL) {
            internalName = infoName;
            m_config->setString(name,infoName);
        }
        layoutData->internalName = internalName;

        QLabel *label = new QLabel(tr("not loaded"));
        label->setToolTip(tr("<html>This plugin is disabled or could not load.</html>"));
        layoutData->infoLabel = label;

        QCheckBox *c = new QCheckBox(name, contents);
        c->setFocusPolicy(Qt::NoFocus);
        c->setToolTip(QString("%1\ninternal name=%2").arg(name).arg(internalName));
        layoutData->loadedCB = c;

        if (layoutData->loaded) {
            label->setText(tr("loaded"));
            label->setToolTip(tr("<html>This plugin does not support asPluginManager.</html>"));
            c->setChecked(true);
            c->setStyleSheet("QCheckBox { font-weight: bold; };");
        }

        QAbstractButton *cv = new QPushButton(infoVersion);
        cv->setFocusPolicy(Qt::NoFocus);
        cv->setFixedHeight(16);
        cv->setFixedWidth(60);
        cv->setEnabled(false);
        cv->setToolTip(tr("No update info available."));
        layoutData->versionBTN = cv;

        layout->addWidget(c, i+1, 0, Qt::AlignLeft | Qt::AlignTop);
        layout->addWidget(cv, i+1, 1, Qt::AlignLeft | Qt::AlignTop);
        layout->setHorizontalSpacing(5);
        QWidget *boxWidget = new QWidget();
        layoutData->boxWidget = boxWidget;
        QGridLayout *boxLayout = new QGridLayout(boxWidget);
        layoutData->boxLayout = boxLayout;
        boxLayout->setHorizontalSpacing(5);
        boxLayout->setVerticalSpacing(0);
        boxLayout->setMargin(0);
        boxWidget->setLayout(boxLayout);
        layout->addWidget(label, i+1, 2, Qt::AlignLeft);
        layout->addWidget(boxWidget, i+1, 2, Qt::AlignLeft);
        layout->setRowStretch(i+1,0);
        connect(c, SIGNAL( clicked() ), SLOT( handleClick() ) );
        height += c->height();
        c->setProperty("internalName", QVariant(internalName));
        layout->setRowStretch(i+2,1);

        m_layoutData.insert(internalName, layoutData);

        if (layoutData->loaded && m_config->checkForUpdates()) {
            this->checkForUpdates(infoId, infoSDK, infoVersion);
        }

    }
    layout->setColumnStretch(0,1);
    layout->setColumnStretch(1,0);
    layout->setColumnStretch(2,0);
    uiWidget->setMinimumSize(100, min(height, m_config->toolBoxHeight()));

    connect( m_hub,
                  SIGNAL( hotnessChanged( const PluginImageSettings & ) ),
                  SLOT( handleHotnessChanged( const PluginImageSettings & ) ) );

    connect( m_hub,
                  SIGNAL( settingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ),
                  SLOT( handleSettingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ) );

}

void asPluginManager::handleClick() {

    QCheckBox *c = (QCheckBox*)QObject::sender();
    bool enable = c->isChecked();
    QString who = c->text();
    if (enable) {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin.off",
                           m_dir->absoluteFilePath(who) + ".afplugin")) c->setChecked(false);
        qDebug() << "asPluginManager: enabled plugin" << who << "for next restart.";
    } else {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin",
                      m_dir->absoluteFilePath(who) + ".afplugin.off")) c->setChecked(true);
        if (who.startsWith("asPluginManager")) {
            QMessageBox::information(NULL, "asPluginManager", tr("You disabled asPluginManager itself."));
        }
        qDebug() << "asPluginManager: disabled plugin" << who << "for next restart.";
    }

}

QList<QString> asPluginManager::toolFiles() {

    return QList<QString>();
}

QList<QWidget*> asPluginManager::toolWidgets() {

    QList<QWidget*> lstWidgets;
    return lstWidgets;
}

bool asPluginManager::registerFilters() {

    return true;
}

bool asPluginManager::registerOptions() {

    return true;
}

bool asPluginManager::finish() {

    return true;
}

void asPluginManager::handleHotnessChanged( const PluginImageSettings &options ) {
    static bool firstRun = true;
    // ask the plugins for their ToolData if not done already
    if (firstRun) {
        QHashIterator<QString, LayoutData*> i(m_layoutData);
        while (i.hasNext()) {
            LayoutData *layoutData = i.next().value();
            layoutData->boxWidget->setVisible(false);
            QString internalName = layoutData->internalName;
            QString dataName = QString("%1:ToolData").arg(internalName);
            qDebug() << "asPluginManager: start PluginData" << dataName;
            m_hub->startPluginData(dataName);
        }
        firstRun = false;
    } else {
        QHashIterator<QString, LayoutData*> i(m_layoutData);
        while (i.hasNext()) {
            LayoutData *layoutData = i.next().value();
            if (layoutData->toolData) {
                layoutData->infoLabel->setVisible(false);
            }
            layoutData->boxWidget->setVisible(true);
        }
        for (int layer = 0; layer<options.count(); layer++) {
            checkOptions(options, layer);
        }
    }
}


void asPluginManager::checkOptions(const PluginImageSettings &options, int layer) {
    if (options.options(layer)) {
        // qDebug() << "asPluginManager: checking in layer" << layer;
        QHashIterator<QString, LayoutData*> i(m_layoutData);
        while (i.hasNext()) {
            LayoutData *layoutData = i.next().value();
            if (layoutData->toolData != NULL) {
                QListIterator<Option> idIter(layoutData->toolData->enabledIds);
                while (idIter.hasNext()) {
                    Option option = idIter.next();
                    QHash<int, QCheckBox*>::const_iterator iter = layoutData->enabledCBHash.find(option.id);
                    if (iter != layoutData->enabledCBHash.end() && iter.key() == option.id) {
                        bool ok;
                        bool enabled = options.options(0)->getBool(option.id, layoutData->toolData->ownerId, ok);
                        if (ok) {
                            QCheckBox *cb = iter.value();
                            cb->setChecked(enabled);
                        }
                    }
                }
            }
            layoutData->boxLayout->update();
        }
    }
}

void asPluginManager::handleSettingsChanged( const PluginImageSettings &options,  const PluginImageSettings &changed, int layer ) {

    Q_UNUSED(changed);
    checkOptions(options, layer);

}

void asPluginManager::handleDataComplete(const QString &dataName, const PluginData *data) {

    if (dataName.endsWith(":ToolData")) {
        const ToolData *toolData = (const ToolData*)(data);
        if (toolData) {
            ToolData *ourToolData = new ToolData(*toolData); // migration from ... -> v2
            if (ourToolData->version >= 2) {
                qDebug() << "asPluginManager: data complete v2" << dataName << "OwnerId =" << ourToolData->ownerId;
                LayoutData *layoutData = m_layoutData.find(ourToolData->owner).value();
                layoutData->toolData = ourToolData;
                layoutData->loadedCB->setToolTip(QString("%1\ninternal name=%2\nidentifier=%3\nid=%4")
                    .arg(layoutData->name).arg(layoutData->internalName)
                    .arg(layoutData->infoId).arg(layoutData->toolData->ownerId));
                QListIterator<Option> options(ourToolData->enabledIds);
                int i = 0;
                while (options.hasNext()) {
                    Option option = options.next();
                    QCheckBox *cb = new QCheckBox(option.shortName.isEmpty() ? tr("enable") : option.shortName);
                    cb->setToolTip(option.longName.isEmpty() ? option.hint : option.longName);
                    cb->setProperty("internalName", ourToolData->owner);
                    cb->setProperty("ownerId", ourToolData->ownerId);
                    cb->setProperty("optionId", option.id);
                    layoutData->enabledCBHash.insert(option.id, cb);
                    cb->setFocusPolicy(Qt::NoFocus);
                    cb->setStyleSheet("QCheckBox::indicator { width:10px; height:10px; spacing 5px; }");
                    layoutData->boxLayout->addWidget(cb, i/2, i%2);
                    connect(cb, SIGNAL(clicked()), this, SLOT(enablerClicked()));
                    i++;
                }
                layoutData->infoLabel->setText(tr("wait for HC"));
                layoutData->infoLabel->setToolTip(tr("<html>Info available after next image selection.</html>"));
                layoutData->boxLayout->update();
            } else {
                qDebug() << "asPluginManager: old ToolData version:" << toolData->version << "for" << dataName;
            }
        } else {
            qDebug() << "asPluginManager: data complete" << dataName << "got NULL as data.";
        }
    }
}

void asPluginManager::handleDataInvalid(const QString &dataName) {

    qDebug() << "asPluginManager::handleDataInvalid";

    if (dataName.endsWith(":ToolData")) {
        qDebug() << "asPluginManager: data invalid" << dataName;
    }
}

PluginDependency *asPluginManager::createDependency(const QString &depName) {

    if (depName == "ToolData") {
        ToolData *toolData = new ToolData();
        if (toolData) {
            toolData->owner = name();
            toolData->group = group();
            toolData->ownerId = pluginId();
            toolData->groupId = groupId();
            qDebug() << "asPluginManager: createDependency ToolData" << toolData;
            return toolData;
        }
    }

    qDebug() << "asPluginManager: createDependency NULL for" << depName;

    return NULL;

}

void asPluginManager::webInfosReady() {

    WebInfos *webInfos = (WebInfos*)(sender());

    qDebug() << "asPluginManager::webInfosReady" << webInfos->identifier()
             << "web:" << webInfos->webVersion() << "installed:" << webInfos->installedVersion();
    qDebug() << "m_layoutData size = " << m_layoutData.size();

    QHashIterator<QString, LayoutData*> i(m_layoutData);
    while (i.hasNext()) {
        LayoutData *layoutData = i.next().value();
        if (layoutData->infoId == webInfos->identifier()) {
            QAbstractButton *c = layoutData->versionBTN;
            qDebug() << "VersionButton =" << c;
            c->setStyleSheet("QAbstractButton { color : rgb(0, 80, 0); font-weight : bold; }");
            c->setToolTip(tr("<html>No newer version available.</html>"));
            if (webInfos->isWebNewer()) {
                QString text = QString(tr("There is a newer version of %1 available. "
                                       "It is version %2. You are running %3. "
                                       "You can download it under the following url: <a href='%4'>%4</a>"))
                               .arg(webInfos->name(), webInfos->webVersion(), webInfos->installedVersion(), webInfos->link());
                c->setStyleSheet("QAbstractButton { color : rgb(100, 0, 0); font-weight : bold; }");
                c->setText(tr("update"));
                c->setToolTip(text);
                c->setEnabled(true);
                connect(c, SIGNAL(clicked()), SLOT(handleClickForUpdate()));
            } else {
                if (WebInfos::formatVersion(webInfos->installedVersion()) > WebInfos::formatVersion(webInfos->webVersion())) {
                    c->setStyleSheet("QAbstractButton { color : rgb(20, 0, 100); font-weight : bold; }");
                    c->setToolTip(tr("<html>Newer version installed locally.</html>"));
                }
            }
        }
    }
    delete webInfos;
}

void asPluginManager::handleClickForUpdate() {
    QAbstractButton *button = (QAbstractButton*)sender();
    QMessageBox::information(NULL, tr("Update-Info from asPluginManager"), button->toolTip());
}

void asPluginManager::enablerClicked() {
    // qDebug() << "asPluginManager: enabler clicked";
    QCheckBox *cc = (QCheckBox*)sender();
    int ownerId = cc->property("ownerId").toInt();
    int optionId = cc->property("optionId").toInt();
    PluginOptionList *optionList = m_hub->beginSettingsChange("asPluginManager enable/disable other plugin.");
    if (optionList != NULL) {
        optionList->setBool(optionId, ownerId, cc->isChecked());
        m_hub->endSettingChange();
    }
}
