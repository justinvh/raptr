#define SDL_MAIN_HANDLED

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <thread>

#include <raptr/network/server.hpp>
#include <raptr/editor/mainwindow.hpp>

namespace {
std::shared_ptr<raptr::Game> game;
}

void run_game()
{
  raptr::Server server("127.0.0.1:7272");
  server.fps = 20;

  std::string game_root = "../../game";
  game = raptr::Game::create(game_root);
  game->toggle_editor();
  server.attach(game);

  if (!server.connect()) {
    return;
  }

  server.run();
}

int main(int argc, char *argv[])
{
  //Q_INIT_RESOURCE(application);
  std::thread game_thread(run_game);

  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("Raptr");
  QCoreApplication::setApplicationName("Editor");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);
  QCommandLineParser parser;

  parser.setApplicationDescription(QCoreApplication::applicationName());
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("file", "The map to open.");
  parser.process(app);

  MainWindow mainWin;
  if (!parser.positionalArguments().isEmpty())
    mainWin.loadFile(parser.positionalArguments().first());

  app.exec();
  game->shutdown = true;
  game_thread.join();
}