#include "application.h"
#include "qdebug.h"
#include "tools/tool.h"
#include "qaction.h"
#include "mainwindow.h"
#include "qstatusbar.h"

#include "tools/edgetool.h"
#include "tools/navigationtool.h"
#include "tools/selectiontool.h"
#include "tools/polygontool.h"

#include "commands/setpropertycommand.h"
#include "commands/translatecommand.h"
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QSurface>
#include <QOffscreenSurface>
#include <QLibrary>

//using namespace NSX;

//static Application *application;

Application::Application(int argc, char *argv[]):
    QApplication(argc, argv)
{
    //application = this;
}

Application &Application::instance()
{
   static Application app;
   return app;
   //return static_cast<Application*>(QCoreApplication::instance());
}

void Application::init()
{
  // QOpenGLContext *oglc = QOpenGLContext::globalShareContext();

// qDebug() << offscreen->isValid();


   // qDebug() << aaa->isValid();

   // QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

  //  f->initializeOpenGLFunctions();

  //  qDebug() << oglc->isValid();

 //   oglc->functions()->glActiveTexture(GL_TEXTURE1);

//    oglc->currentContext()->functions();
  //  oglc->functions()->initializeOpenGLFunctions();

    //QOpenGLFunctions::initializeOpenGLFunctions();

//   QOpenGLContext  *m_context = new QOpenGLContext;
//    m_context->create();

      //  m_context->makeCurrent();

//   QOpenGLShaderProgram *test =  new QOpenGLShaderProgram(this);
//    test->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/selection.vert");
//    test->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/selection.frag");
//    test->link();

   // test->bind();

   // qDebug() << test->isLinked();

//set opengl

    //register tools


    //register commands
    commands_factory["Translate"] = &t_createCommand<TranslateCommand>;

    //register hotkeys
//    std::map<Qt::Key, QString> hotkeys;
//    hotkeys[Qt::Key_1] = "SplitEdgeTool";
//    hotkeys[Qt::Key_S] = "NavigationTool";

    hotkey_bindings[Qt::Key_1] = QVariant::fromValue((Tool*) new EdgeTool);
    hotkey_bindings[Qt::Key_2] = QVariant::fromValue((Tool*) new PolygonTool);
    hotkey_bindings[Qt::Key_S] = QVariant::fromValue((Tool*) new NavigationTool);

    Tool* t = new SelectionTool;
    hotkey_bindings[Qt::Key_Space] = QVariant::fromValue(t);
    hotkey_bindings[Qt::Key_V] = QVariant::fromValue(t);
    hotkey_bindings[Qt::Key_C] = QVariant::fromValue(t);
    hotkey_bindings[Qt::Key_X] = QVariant::fromValue(t);

//set viewport
    mainWindow->viewport->default_tool = t;
    mainWindow->viewport->active_tool = t;

    t = new TransformTool;
    hotkey_bindings[Qt::Key_W] = QVariant::fromValue(t);
    hotkey_bindings[Qt::Key_E] = QVariant::fromValue(t);
    hotkey_bindings[Qt::Key_R] = QVariant::fromValue(t);

}

void Application::registerPlugin(QString filename)
{
    QLibrary library(filename);
    library.load();
    if( library.isLoaded() )
    {
        qDebug() << "dll loaded";
        t_createCommand_fp createCommand = (t_createCommand_fp)library.resolve( "create" );
        if (createCommand) {
            Command* my_command = createCommand();
            auto it = commands_factory.find(my_command->commandName());
            if (it != commands_factory.end() ) {
                qDebug() << "Plugin name already exist";
            } else {
                commands_factory[my_command->commandName()] = createCommand;
            }
            // TODO delete instance
        } else {
            qDebug() << "create function not found";
        }
    } else {
        qDebug() << "dll not found";
    }
}

void Application::activateTool(QString tool_name)
{

}

void Application::activateTool(Tool *tool)
{
    if (mainWindow->viewport->active_tool != nullptr) mainWindow->viewport->active_tool->deactivate();
    mainWindow->viewport->previousTool = mainWindow->viewport->active_tool;
    mainWindow->viewport->active_tool = tool;
    tool->activate();

}

void Application::executeCommand(QString command_name, std::vector<QVariant> args)
{
    qDebug() << "executeCommand";

    auto it = commands_factory.find(command_name);
    if (it != commands_factory.end() ) {
        Command *c = it->second();
        c->execute(args);
        if (c->undoable()) {
            undoStack.push_back(c);
        }
    } else {
        mainWindow->statusBar()->showMessage("Command not found", 3000);
    }
}

void Application::undo()
{
    if (!undoStack.empty()) {
        Command *c = undoStack.back();
        mainWindow->statusBar()->showMessage(c->commandName() + " undone", 3000);
        c->undo();
        undoStack.pop_back();
    }
}

