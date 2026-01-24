#include "mainwindow.h"
#include "scenetreemodel.h"
#include "ui_mainwindow.h"
#include "application.h"
#include "transform3d.h"
#include "uveditor.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("NSX 0.1");
    //ui->statusbar->hide();

    Application &application = Application::instance();

    viewport = new Viewport(this);
    this->setCentralWidget(viewport);

    QAction *action;
    QMenu *menu;

// file menu
    menu = menuBar()->addMenu(tr("&File"));

    action = new QAction(tr("&New"), this);
    //connect(action, &QAction::triggered, this, &MainWindow::triggerOpenScene());
    menu->addAction(action);

    action = new QAction(tr("&Open..."), this);
    connect(action, &QAction::triggered, this, &MainWindow::loadModel);
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("&Save"), this);
    connect(action, &QAction::triggered, this, &MainWindow::saveModel);
    menu->addAction(action);

    action = new QAction(tr("&Save as..."), this);
    //connect(action, &QAction::triggered, this, &MainWindow::triggerOpenScene());
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("&Import..."), this);
    //connect(action, &QAction::triggered, this, &MainWindow::triggerOpenScene());
    menu->addAction(action);

    action = new QAction(tr("&Export..."), this);
    //connect(action, &QAction::triggered, this, &MainWindow::triggerOpenScene());
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("&Exit"), this);
    connect(action, &QAction::triggered, this, &MainWindow::close);
    menu->addAction(action);

//---------------------------------- edit menu --------------------------------------------------------
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
//    QAction *testCommand = new QAction(tr("&testCommand"), this);
//    connect(testCommand, &QAction::triggered, this, &MainWindow::testCommand);
//    editMenu->addAction(testCommand);

    QAction *undoAction = new QAction(tr("&undo"), this);
    connect(undoAction, &QAction::triggered, &Application::instance(), &Application::undo);
    editMenu->addAction(undoAction);

    QAction *registerCommand = new QAction(tr("&Register command"), this);
    connect(registerCommand, &QAction::triggered, this, &MainWindow::registerCommand);
    editMenu->addAction(registerCommand);


//---------------------------------- view menu --------------------------------------------------------

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *openSceneExplorer = new QAction(tr("&Scene explorer"), this);
    connect(openSceneExplorer, &QAction::triggered, this, &MainWindow::openSceneExplorer);
    viewMenu->addAction(openSceneExplorer);

    action = new QAction(tr("&UV Editor"), this);
    connect(action, &QAction::triggered, this, &MainWindow::openUVEditor);
    viewMenu->addAction(action);


//---------------------------------- modeling menu --------------------------------------------------------

    QMenu *modelingMenu = menuBar()->addMenu(tr("&Model"));
    modelingMenu->setObjectName("model");

    QMenu *modelingMenuCreateMesh = modelingMenu->addMenu(tr("&Create"));

    QAction *add_cube_action = new QAction(tr("&Add cube"), this);
    connect(add_cube_action, &QAction::triggered,
            this, [=](){this->viewport->AddObject3D(0);
    });
    modelingMenuCreateMesh->addAction(add_cube_action);

    QMenu *a = modelingMenu->addMenu(tr("&Deform"));
    QMenu *b = modelingMenu->addMenu(tr("&Edit"));

    modelingMenu->addSeparator();

//animation menu
    QMenu *animationMenu = menuBar()->addMenu(tr("&Animate"));

    QAction *add_locator_action = new QAction(tr("&Add locator"), this);
    connect(add_locator_action, &QAction::triggered,
            this, [=](){this->viewport->AddObject3D(1);
    });
    animationMenu->addAction(add_locator_action);


//shading menu
    QMenu *renderMenu = menuBar()->addMenu(tr("&Shade"));

    //QDockWidget *wdg = new QDockWidget();
   // wdg->show();
   connect(ui->Addcube, &QPushButton::clicked,
                 this, [=](){this->viewport->AddObject3D(0);
    });

  //  fileMenu->find()

    viewport->setFocus();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::openUVEditor() {
    UVEditor *editor = new UVEditor(this);
    editor->resize(1000, 1000);
    //pb->set
    editor->setWindowFlag(Qt::Window);
    editor->setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    //pb->setText("TEST");
    editor->show();
}


void MainWindow::openSceneExplorer() {
    if (sceneExplorerWidget == nullptr) {
        sceneExplorerWidget = new QDockWidget(this);
        sceneExplorerWidget->resize(400, 600);
        sceneExplorerWidget->setWindowFlag(Qt::Window);
        sceneExplorerWidget->setWindowFlag(Qt::WindowMinMaxButtonsHint, false);

        QTreeView *view = new QTreeView(this);
        //view->setRootIsDecorated(false);
        view->setHeaderHidden(true);
        //view->isRowHidden(true);

        sceneExplorerWidget->setWidget(view);

      //  QVBoxLayout *layout = new QVBoxLayout(this);
        //layout->addWidget(view);
      //  sceneExplorerWidget->setLayout(layout);

        //TODO model
        SceneTreeModel* model = new SceneTreeModel(&viewport->scene/*, sceneExplorerWidget*/);
        view->setModel(model);
        //view->setRootIndex();

    }
    sceneExplorerWidget->show();

}

void MainWindow::registerCommand()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter("*.dll");
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    if (fileNames.empty()) return;

    Application::instance().registerPlugin(fileNames.at(0));
}

void MainWindow::testCommand()
{
    Application &app = Application::instance();
    if (app.selection.empty()) {
        qDebug() << "empty";
        return;
    }

    std::vector<QVariant> args;
    args.push_back(app.selection[0]);
    args.push_back("local_transform");

    Transform3D bo;
    args.push_back(QVariant::fromValue(bo));

    Application::instance().executeCommand("SetProperty", args);
    qDebug() << "end command";
}


void MainWindow::loadModel()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter("*.obj");
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    if (fileNames.empty()) return;

    QFile file(fileNames.at(0));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;


    std::vector<Vertex> vertex_position;
    std::vector<std::vector<int>> face_indices;

    QTextStream in(&file);
    QString line = in.readLine();

    qDebug() << "start read";

   // vertex_position.reserve(2000);
   // face_indices.reserve(2000);

    while (!line.isNull()) {

        QStringList line_array = line.split(' ');
        if (line_array[0] == 'v') {
            Vertex position;
            position.position.setX(line_array[1].toFloat());
            position.position.setY(line_array[2].toFloat());
            position.position.setZ(line_array[3].toFloat());
            vertex_position.push_back(position);
        } else if (line_array[0] == 'f') {
           // qDebug() << "face";
            std::vector<int> indices;
            for(int i = 1; i < line_array.size(); i++) {
                QStringList face_data = line_array[i].split('/');
                if (face_data[0] == "") continue; // last character space?
                //qDebug() << face_data[0];
                indices.push_back(face_data[0].toInt() - 1);
            }
            face_indices.push_back(indices);
        }

        line = in.readLine();
    }

   qDebug() << "OK read";

  //  return;

    Mesh* m = new Mesh();
    m->setObjectName("Polygon Mesh");

    m->geometry = new Geometry;
    m->geometry->fromVertexFaceArray(vertex_position, face_indices);

   qDebug() << "OK faces";

    m->geometry->updateFaceNormals();

    qDebug() << "OK 112";

    m->geometry->updateHalfEdgeNormals();

    qDebug() << "OK 3333333";
    m->geometry->triangulate();
    //m->geometry_dirty = true;
    qDebug() << "OK geom";

    m->init();

    m->setMaterial(viewport->material);
    viewport->scene.addObject3D(m);
    viewport->scene.updateObjectList();

    qDebug() << "OK import";

//    Transform3D t;
//    t.setTranslation(1,1,1);
//    t.setRotation(QQuaternion::fromEulerAngles(-40,-405,-45)); //z x y
//    m->setLocalTransform(t);
}


void MainWindow::saveModel()
{

}


