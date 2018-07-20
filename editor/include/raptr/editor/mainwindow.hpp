#pragma once

#include <QMainWindow>
#include <raptr/game/game.hpp>
#include <raptr/renderer/renderer.hpp>

class QAction;
class QMenu;
class QPlainTextEdit;
class QGridLayout;
class QSessionManager;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow();

  void loadFile(const QString &fileName);

protected:
  void closeEvent(QCloseEvent *event) override;

  private slots:
  void newFile();
  void open();
  bool save();
  bool saveAs();
  void about();
  void documentWasModified();
  #ifndef QT_NO_SESSIONMANAGER
  void commitData(QSessionManager &);
  #endif

private:
  void createActions();
  void createStatusBar();
  void readSettings();
  void writeSettings();
  bool maybeSave();
  bool saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);
  QString strippedName(const QString &fullFileName);

  QGridLayout* gridLayout;
  QWidget parent;
  QWidget game_widget;
  QWidget editor_widget;
  SDL_Window* window;
  SDL_Renderer* renderer;

  QString curFile;
};