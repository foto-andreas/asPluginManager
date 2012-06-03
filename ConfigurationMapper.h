#ifndef CONFIGURATIONMAPPER_H
#define CONFIGURATIONMAPPER_H

#include <QString>
#include <QObject>

#include "ConfigFile.h"

#include "PluginHub.h"
#include "PluginDependency.h"
#include "PluginImageSettings.h"
#include "PluginOptionList.h"

#define ASPM_CHECK_FOR_UPDATES "ASPM_CHECK_FOR_UPDATES"
#define ASPM_TOOLBOX_HEIGHT "ASPM_TOOLBOX_HEIGHT"

class ConfigurationMapper : public QObject {
        Q_OBJECT

    public:

        ConfigurationMapper(QString filename);

        bool getBool(const QString key, bool def);
        QString getString(const QString key, QString def);
        int getInt(const QString key, int def);

        void setString(const QString key, const QString val);

        bool isPluginEnabled(QString name, PluginOptionList *options, PluginHub *hub);

        bool checkForUpdates();

        int toolBoxHeight();

private:
        ConfigFile *m_cf;


};

#endif // CONFIGURATIONMAPPER_H
