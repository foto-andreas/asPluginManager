#include "ConfigurationMapper.h"

#include <QDebug>
#include <QString>
#include <QStringList>

#include "PluginHub.h"
#include "PluginDependency.h"
#include "PluginImageSettings.h"
#include "PluginOptionList.h"

#include "ConfigFile.h"

ConfigurationMapper::ConfigurationMapper(QString fileName) {
    m_cf = new ConfigFile(fileName);
    m_cf->autoWrite(true);
}

bool ConfigurationMapper::getBool(const QString key, bool def = false) {
    QString val = m_cf->getValue(key);
    if (val == 0) return def;
    return val == "true";
}

QString ConfigurationMapper::getString(const QString key, QString def = "") {
    QString val = m_cf->getValue(key);
    if (val == 0) return def;
    return val;
}

int ConfigurationMapper::getInt(const QString key, int def = 0) {
    QString val = m_cf->getValue(key);
    if (val == 0) return def;
    return val.toInt();
}

void ConfigurationMapper::setString(const QString key, const QString val) {
    m_cf->setValue(key, val);
}

bool ConfigurationMapper::isPluginEnabled(QString name, PluginOptionList *options, PluginHub *hub) {
    QString optionsInConf = this->getString(name);
    QStringList optionList = optionsInConf.split(",");
    for (int i=0; i<optionList.length(); i++) {
        if (!optionList[i].isEmpty()) {
            int optionID = hub->optionIdForName(optionList[i],0);
            qDebug() << "asPluginManager: " << optionList[i] << "=" << optionID;
            if (options) {
                bool ok;
                if (options->getBool(optionID,0,ok)) {
                    qDebug() << "asPluginManager: Plugin" << name << "is enabled.";
                    return true;
                }
            }
        }
    }
    return false;
}

bool ConfigurationMapper::checkForUpdates() {
    return getBool(ASPM_CHECK_FOR_UPDATES, true);
}

int ConfigurationMapper::toolBoxHeight() {
    return getInt(ASPM_TOOLBOX_HEIGHT, 400);
}
