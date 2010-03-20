/**
 * \file MainWindow.cpp
 * \author Denis Martinez
 */

#include "MainWindow.h"

#include <QTabWidget>
#include <QLabel>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QWebSecurityOrigin>
#include <QDebug>

#include "EditorFactory.h"
#include "Browser.h"
#include "ConfigDialog.h"
#include "DeviceChooser.h"
#include "BoardChooser.h"
#include "../env/Device.h"
#include "../env/Board.h"
#include "../env/Builder.h"
#include "../env/Settings.h"
#include "../env/ProjectHistory.h"
#include "IDEApplication.h"

#include "ui_AboutDialog.h"

MainWindow::MainWindow()
    : QMainWindow()
{
    ui.setupUi(this);
    ui.actionAbout->setText(ui.actionAbout->text().arg(PROJECT_NAME));

    ui.dockWidget->hide();
}

void MainWindow::initialize()
{
    createBrowserAndTabs();

    createDeviceChooser();
    createBoardChooser();

    setupActions();
}

void MainWindow::setupActions()
{
    buildActions = new QActionGroup(this);
    buildActions->addAction(ui.action_Build);
    buildActions->addAction(ui.action_Upload);

    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(ui.action_New, SIGNAL(triggered()), this, SLOT(newProject()));
    connect(ui.action_Open, SIGNAL(triggered()), this, SLOT(open()));
    connect(ui.action_Save, SIGNAL(triggered()), this, SLOT(save()));
    connect(ui.action_Close, SIGNAL(triggered()), this, SLOT(closeTab()));
    connect(ui.action_Copy, SIGNAL(triggered()), this, SLOT(copy()));
    connect(ui.action_Cut, SIGNAL(triggered()), this, SLOT(cut()));
    connect(ui.action_Paste, SIGNAL(triggered()), this, SLOT(paste()));
    connect(ui.action_Build, SIGNAL(triggered()), this, SLOT(build()));
    connect(ui.action_Upload, SIGNAL(triggered()), this, SLOT(upload()));
    connect(ui.actionToggle_dock, SIGNAL(triggered()), this, SLOT(toggleDock()));
    connect(ui.actionGo_to_the_next_tab, SIGNAL(triggered()), this, SLOT(nextTab()));
    connect(ui.actionGo_to_the_previous_tab, SIGNAL(triggered()), this, SLOT(previousTab()));
    connect(ui.action_Configure_the_IDE, SIGNAL(triggered()), this, SLOT(configure()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(browser, SIGNAL(newProjectRequested()), this, SLOT(newProject()));
    connect(browser, SIGNAL(newProjectRequested(const QString &, const QString &)), this, SLOT(newProject(const QString &, const QString &)));
    connect(browser, SIGNAL(openProjectRequested()), this, SLOT(open()));
    connect(browser, SIGNAL(openProjectRequested(const QString &)), this, SLOT(open(const QString &)));

    connect(ideApp->projectHistory(), SIGNAL(historyUpdated(QString)), browser, SLOT(refresh()));

    connect(ideApp->settings(), SIGNAL(fontChanged(const QFont &)), this, SLOT(setFont(const QFont &)));
}

void MainWindow::createBrowserAndTabs()
{
    tabWidget = new QTabWidget;
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->addAction(ui.actionGo_to_the_next_tab);
    tabWidget->addAction(ui.actionGo_to_the_previous_tab);

    QWebSecurityOrigin::addLocalScheme("ide");
    browser = new Browser;
    tabWidget->addTab(browser, tr("Browser"));
    setCentralWidget(tabWidget);
    browser->quickstart();
}

void MainWindow::nextTab()
{
    int index = tabWidget->currentIndex();
    int count = tabWidget->count();
    if (index != -1)
    {
        index = (index + 1) % count;
        tabWidget->setCurrentIndex(index);
    }
}

void MainWindow::previousTab()
{
    int index = tabWidget->currentIndex();
    int count = tabWidget->count();
    if (index != -1)
    {
        index--;
        tabWidget->setCurrentIndex((index < 0) ? (count - 1) : index);
    }
}

void MainWindow::createDeviceChooser()
{
    deviceAction = ui.deviceToolBar->addAction(QIcon(":/images/atmega168_icon.svg"), tr("Device"));
    deviceChooser = new DeviceChooser(this);
    connect(deviceAction, SIGNAL(triggered()), this, SLOT(chooseDevice()));
    connect(deviceChooser, SIGNAL(deviceChosen(const QString &)), SLOT(setDevice(const QString &)));
}

void MainWindow::chooseDevice()
{
    QWidget *w = ui.deviceToolBar->widgetForAction(deviceAction);
    int x = ui.deviceToolBar->mapToGlobal(QPoint(ui.deviceToolBar->width(), 0)).x();
    int y = w->mapToGlobal(QPoint(0, 0)).y();
    deviceChooser->refresh();
    deviceChooser->exec(QPoint(x, y));
}

void MainWindow::createBoardChooser()
{
    boardAction = ui.deviceToolBar->addAction(QIcon(":/images/arduino_diecimila.svg"), tr("Board"));
    boardChooser = new BoardChooser(this);
    connect(boardAction, SIGNAL(triggered()), this, SLOT(chooseBoard()));
    connect(boardChooser, SIGNAL(boardChosen(const QString &)), SLOT(setBoard(const QString &)));
}

void MainWindow::chooseBoard()
{
    QWidget *w = ui.deviceToolBar->widgetForAction(boardAction);
    int x = ui.deviceToolBar->mapToGlobal(QPoint(ui.deviceToolBar->width(), 0)).x();
    int y = w->mapToGlobal(QPoint(0, 0)).y();
    boardChooser->refresh();
    boardChooser->exec(QPoint(x, y));
}

void MainWindow::newProject(const QString &code, const QString &name, Editor **pEditor)
{
    Editor *editor;
    if (! code.isEmpty())
        editor = EditorFactory::createEditor(code);
    else
        editor = EditorFactory::createEditor();

    int tab = tabWidget->addTab(editor, (name.isNull()) ? createUniqueName("New project") : name);
    tabWidget->setCurrentIndex(tab);

    if (pEditor)
        *pEditor = editor;
}

QString MainWindow::createUniqueName(const QString &name)
{
    if (! names.contains(name))
    {
        names.append(name);
        return name;
    }
    else
    {
        QString newName;
        for (int i = 1;; ++i)
        {
            newName = QString("%0 (%1)").arg(name).arg(i);
            if (! names.contains(newName))
            {
                names.append(newName);
                return newName;
            }
        }
    }
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = tabWidget->currentIndex();

    QWidget *w = tabWidget->widget(index);
    if (w != browser)
    {
        Editor *editor = dynamic_cast<Editor *>(w);
        if (editor)
        {
            bool close = (! editor->isModified()) ||
                (QMessageBox::question(
                    this,
                    tr("Close project"),
                    tr("This project has unsaved changes.<br />Are you sure you want to close it?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes) == QMessageBox::Yes);
            if (close)
            {
                names.removeOne(tabWidget->tabText(index));
                tabWidget->removeTab(index);
            }
        }
    }
}

Editor *MainWindow::currentEditor()
{
    return qobject_cast<Editor *>(tabWidget->currentWidget());
}

QList<Editor *> MainWindow::editors()
{
    QList<Editor *> editors;
    Editor *editor;
    for (int i = 0; i < tabWidget->count(); i++)
    {
        editor = qobject_cast<Editor *>(tabWidget->widget(i));
        if (editor != NULL)
            editors << editor;
    }
    return editors;
}

void MainWindow::open(const QString &_fileName)
{
    QString fileName(_fileName);

    if (fileName.isEmpty())
    {
        fileName = QFileDialog::getOpenFileName(this, tr("Open project"), ideApp->settings()->sketchPath(), tr("Arduino sketches (*.pde)"));
        if (fileName.isEmpty())
            return;
    }

    QFile file(fileName);
    if (! file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Open error"), tr("The file could not be opened for reading."));
        return;
    }

    // create a new project and obtain the associated editor
    Editor *editor;
    newProject(QString::fromLocal8Bit(file.readAll()), createUniqueName(QFileInfo(fileName).fileName()), &editor);
    editor->setFileName(fileName);
    file.close();

    // update the history
    ideApp->projectHistory()->updateHistory(fileName);
}

void MainWindow::save()
{
    Editor *e = currentEditor();
    if (e)
    {
        QString fileName = e->fileName();
        int index;
        e->save();
        if (fileName != e->fileName())
        {
            // the file name changed, update the tab text
            index = tabWidget->currentIndex();
            names.removeOne(tabWidget->tabText(index));
            tabWidget->setTabText(index, createUniqueName(QFileInfo(e->fileName()).fileName()));
        }

        // update the history
        ideApp->projectHistory()->updateHistory(e->fileName());
    }
}

void MainWindow::copy()
{
    Editor *e = currentEditor();
    if (e) e->copy();
}

void MainWindow::cut()
{
    Editor *e = currentEditor();
    if (e) e->cut();
}

void MainWindow::paste()
{
    Editor *e = currentEditor();
    if (e) e->paste();
}

void MainWindow::setDevice(const QString &device)
{
    qDebug() << "setDevice:" << device;
    ideApp->settings()->setDevicePort(device);
}

void MainWindow::setBoard(const QString &board)
{
    qDebug() << "setBoard:" << board;
    ideApp->settings()->setBoard(board);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < tabWidget->count(); i++)
    {
        Editor *editor = dynamic_cast<Editor *>(tabWidget->widget(i));
        if (editor && editor->isModified())
        {
            bool close = QMessageBox::question(
                this,
                tr("Quit"),
                tr("Some projects in your workspace have unsaved changes.<br />Are you sure you want to quit?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes;
            event->setAccepted(close);
            return;
        }
    }

    event->accept();
}

void MainWindow::build()
{
    Editor *editor = currentEditor();
    if (editor)
    {
        buildActions->setEnabled(false);

        const Board *board = Board::boardInfo(ideApp->settings()->board());
        ui.dockWidget->show();
        ui.outputView->clear();

        Builder builder(*ui.outputView);
        builder.setBoard(board);
        builder.build(editor->text());

        buildActions->setEnabled(true);
    }
}

void MainWindow::upload()
{
    Editor *editor = currentEditor();
    if (editor)
    {
        buildActions->setEnabled(false);

        const Board *board = Board::boardInfo(ideApp->settings()->board());
        QString device = ideApp->settings()->devicePort();
        ui.dockWidget->show();
        ui.outputView->clear();

        Builder builder(*ui.outputView);
        builder.setBoard(board);
        builder.setDevice(device);
        builder.build(editor->text(), true);

        buildActions->setEnabled(true);
    }
}

void MainWindow::toggleDock()
{
    ui.dockWidget->setVisible(! ui.dockWidget->isVisible());
}

void MainWindow::configure()
{
    ConfigDialog dialog(this);
    dialog.exec();
}

void MainWindow::setFont(const QFont &font)
{
    foreach (Editor *editor, editors())
        editor->setLexerFont(font);
}

void MainWindow::about()
{
    QDialog *dialog = new QDialog(this);
    Ui::AboutDialog ui;
    ui.setupUi(dialog);
    ui.nameLabel->setText(ui.nameLabel->text().arg(PROJECT_NAME));
    ui.urlLabel->setText(ui.urlLabel->text().arg(PROJECT_URL));
    ui.authorsLabel->setText(ui.authorsLabel->text().arg(PROJECT_AUTHORS));
    ui.licenseLabel->setText(ui.licenseLabel->text().arg(PROJECT_LICENSE));
    ui.descriptionLabel->setText(ui.descriptionLabel->text().arg(PROJECT_NAME));
    dialog->exec();
    delete dialog;
}
