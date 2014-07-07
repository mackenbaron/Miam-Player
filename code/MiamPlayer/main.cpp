#include <QApplication>
#include <QSharedMemory>

#include "mainwindow.h"

#define COMPANY "MmeMiamMiam"
#define SOFT "MiamPlayer"
#define VERSION "0.6.10"

#include "miamstyle.h"
#include "plugininfo.h"
// #include "singleapplication.h"
#include <qtsingleapplication/QtSingleApplication>

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(mp);

	qRegisterMetaType<QFileInfo>();
	qRegisterMetaType<PluginInfo>();
	qRegisterMetaTypeStreamOperators<PluginInfo>("PluginInfo");

	QtSingleApplication app(SOFT, argc, argv);

	if (app.isRunning()) {
		for (int i = 0; i < QApplication::arguments().count(); i++) {
			app.sendMessage(QApplication::arguments().at(i));
		}
		return 0;
	}

	app.setStyle(new MiamStyle());
	MainWindow window;
	app.setActivationWindow(&window);

	Settings *settings = Settings::getInstance();
	if (settings->isCustomColors()) {
		app.setPalette(settings->value("customPalette").value<QPalette>());
	}
	app.setOrganizationName(COMPANY);
	app.setApplicationName(SOFT);
	app.setApplicationVersion(VERSION);

	window.init();
	window.show();
	window.loadPlugins();

	// It this application was started from a file (for example)
	if (argc > 1) {
		QStringList args;
		for (int i = 0; i < argc; i++) {
			args << argv[i];
		}
		window.processArgs(args);
	}
	return app.exec();
}
