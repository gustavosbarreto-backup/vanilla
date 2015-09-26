#include "switch.hpp"
#include "const.hpp"

#include "mainwindow.hpp"

#include <QIcon>
#include <QStyle>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "view.hpp"
#include "application.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "gadgets.hpp"
#include "graphicstableview.hpp"
#ifdef QTWEBKIT
#  include "webview.hpp"
#endif
#include "webengineview.hpp"
#include "dialog.hpp"

MainWindow::MainWindow(int id, QWidget *parent)
    : QMainWindow(parent)
{
    m_Index = id;

    m_TreeBank = new TreeBank(this);
    setCentralWidget(m_TreeBank);

    if(m_DialogFrame = Application::GetTemporaryDialogFrame()){
        Application::SetTemporaryDialogFrame(0);
    } else {
        m_DialogFrame = new ModelessDialogFrame();
    }
    m_DialogFrame->show();

    menuBar();

    LoadSettings();

    if(Application::EnableFramelessWindow()){
        setWindowFlags(Qt::FramelessWindowHint);
        m_TitleBar        = new TitleBar                  (this);
        m_NorthWidget     = new MainWindowNorthWidget     (this);
        m_SouthWidget     = new MainWindowSouthWidget     (this);
        m_WestWidget      = new MainWindowWestWidget      (this);
        m_EastWidget      = new MainWindowEastWidget      (this);
        m_NorthWestWidget = new MainWindowNorthWestWidget (this);
        m_NorthEastWidget = new MainWindowNorthEastWidget (this);
        m_SouthWestWidget = new MainWindowSouthWestWidget (this);
        m_SouthEastWidget = new MainWindowSouthEastWidget (this);
        AdjustAllEdgeWidgets();
        connect(Application::GetInstance(), SIGNAL(focusChanged(QWidget*, QWidget*)),
                this, SLOT(UpdateAllEdgeWidgets()));
    } else {
        m_TitleBar        = 0;
        m_NorthWidget     = 0;
        m_SouthWidget     = 0;
        m_WestWidget      = 0;
        m_EastWidget      = 0;
        m_NorthWestWidget = 0;
        m_NorthEastWidget = 0;
        m_SouthWestWidget = 0;
        m_SouthEastWidget = 0;
    }
    show();
}

MainWindow::~MainWindow(){
    m_DialogFrame->deleteLater();
    if(m_TitleBar){
        m_TitleBar        ->deleteLater();
        m_NorthWidget     ->deleteLater();
        m_SouthWidget     ->deleteLater();
        m_WestWidget      ->deleteLater();
        m_EastWidget      ->deleteLater();
        m_NorthWestWidget ->deleteLater();
        m_NorthEastWidget ->deleteLater();
        m_SouthWestWidget ->deleteLater();
        m_SouthEastWidget ->deleteLater();
    }
}

void MainWindow::SetFocus(){
    raise();
    activateWindow();
    if(GetTreeBank()->GetGadgets()->IsActive()){
        GetTreeBank()->GetGadgets()->setFocus(Qt::OtherFocusReason);
    } else if(SharedView view = GetTreeBank()->GetCurrentView()){
        view->setFocus(Qt::OtherFocusReason);
    }
}

int MainWindow::GetIndex(){
    return m_Index;
}

void MainWindow::SaveSettings(){
    QSettings *s = Application::GlobalSettings();
    s->beginGroup(QStringLiteral("mainwindow"));{

        s->setValue(QStringLiteral("tableview%1").arg(m_Index), m_TreeBank->GetGadgets()->GetStat());
        s->setValue(QStringLiteral("notifier%1").arg(m_Index), m_TreeBank->GetNotifier() ? true : false);
        s->setValue(QStringLiteral("receiver%1").arg(m_Index), m_TreeBank->GetReceiver() ? true : false);
        s->setValue(QStringLiteral("menubar%1").arg(m_Index), !IsMenuBarEmpty());
        s->setValue(QStringLiteral("status%1").arg(m_Index), static_cast<int>(windowState()));
        if(!isFullScreen() && !isMaximized() && !isMinimized())
            s->setValue(QStringLiteral("geometry%1").arg(m_Index), geometry());
    }
    s->endGroup();
}

void MainWindow::LoadSettings(){
    QSettings *s = Application::GlobalSettings();
    s->beginGroup(QStringLiteral("mainwindow"));{

        MainWindow *current = Application::GetCurrentWindow();
        QVariant tableview_data = s->value(QStringLiteral("tableview%1").arg(m_Index), QVariant());
        QVariant geometry_data  = s->value(QStringLiteral("geometry%1").arg(m_Index), QVariant());
        QVariant notifier_data  = s->value(QStringLiteral("notifier%1").arg(m_Index), QVariant());
        QVariant receiver_data  = s->value(QStringLiteral("receiver%1").arg(m_Index), QVariant());
        QVariant menubar_data   = s->value(QStringLiteral("menubar%1").arg(m_Index), QVariant());

        if(tableview_data.isNull()){

            QStringList list = current
                ? current->GetTreeBank()->GetGadgets()->GetStat()
                : QStringList()
                  << QStringLiteral("%1").arg(static_cast<int>(GraphicsTableView::Flat))
                  << QStringLiteral("%1").arg(static_cast<int>(GraphicsTableView::Recursive))
                  << QStringLiteral("%1").arg(1.0f);

            m_TreeBank->GetGadgets()->SetStat(list);

        } else if(tableview_data.canConvert<QStringList>()){
            m_TreeBank->GetGadgets()->SetStat
                (tableview_data.toStringList());
        }

        if(geometry_data.isNull()){

            QRect rect = current
                ? current->geometry().translated(20, 20)
                : DEFAULT_WINDOW_RECT;

            setGeometry(rect);

        } else if(geometry_data.canConvert<QRect>()){
            setGeometry(geometry_data.toRect());
        }

        if(notifier_data.isNull()){

            bool notifierEnabled = current
                ? current->GetTreeBank()->GetNotifier()
                : true;

            if(notifierEnabled != static_cast<bool>(GetTreeBank()->GetNotifier()))
                GetTreeBank()->ToggleNotifier();

        } else if(notifier_data.canConvert<bool>()){
            if(notifier_data.toBool() != static_cast<bool>(GetTreeBank()->GetNotifier()))
                GetTreeBank()->ToggleNotifier();
        }

        if(receiver_data.isNull()){

            bool receiverEnabled = current
                ? current->GetTreeBank()->GetReceiver()
                : true;

            if(receiverEnabled != static_cast<bool>(GetTreeBank()->GetReceiver()))
                GetTreeBank()->ToggleReceiver();

        } else if(receiver_data.canConvert<bool>()){
            if(receiver_data.toBool() != static_cast<bool>(GetTreeBank()->GetReceiver()))
                GetTreeBank()->ToggleReceiver();
        }

        if(menubar_data.isNull()){

            bool menubarEnabled = current
                ? !current->IsMenuBarEmpty()
                : true;

            if(menubarEnabled && IsMenuBarEmpty())
                CreateMenuBar();

        } else if(menubar_data.canConvert<bool>()){
            if(menubar_data.toBool() && IsMenuBarEmpty())
                CreateMenuBar();
        }
    }
    s->endGroup();

    QTimer::singleShot(0, [this, s](){

    s->beginGroup(QStringLiteral("mainwindow"));{
        // 'setGeometry' and 'restoreGeometry' are asynchronous API.
        bool contains = false;
        QDesktopWidget desktop;
        for(int i = 0; i < desktop.screenCount(); i++){
            if(desktop.screenGeometry(i).intersects(geometry())){
                contains = true;
                break;
            }
        }
        if(!contains) setGeometry(DEFAULT_WINDOW_RECT);

        QVariant status = s->value(QStringLiteral("status%1").arg(m_Index), QVariant());

        if(status.isNull()){
            /* do nothing. */
        } else if(status.canConvert<int>()){
            setWindowState(static_cast<Qt::WindowStates>(status.toInt()));
        }
    }
    s->endGroup();
    });
}

void MainWindow::RemoveSettings(){
    QSettings *s = Application::GlobalSettings();
    s->beginGroup(QStringLiteral("mainwindow"));
    s->remove(QStringLiteral("tableview%1").arg(m_Index));
    s->remove(QStringLiteral( "geometry%1").arg(m_Index));
    s->remove(QStringLiteral( "notifier%1").arg(m_Index));
    s->remove(QStringLiteral( "receiver%1").arg(m_Index));
    s->remove(QStringLiteral(  "menubar%1").arg(m_Index));
    s->remove(QStringLiteral(   "status%1").arg(m_Index));
    s->endGroup();
    s->sync();
}

TreeBank *MainWindow::GetTreeBank(){
    return m_TreeBank;
}

ModelessDialogFrame *MainWindow::DialogFrame(){
    return m_DialogFrame;
}

bool MainWindow::IsMenuBarEmpty(){
    return menuBar()->actions().isEmpty();
}

void MainWindow::ClearMenuBar(){
    menuBar()->clear();
}

void MainWindow::CreateMenuBar(){
    menuBar()->addMenu(m_TreeBank->ApplicationMenu(true));
    menuBar()->addMenu(m_TreeBank->NodeMenu());
    menuBar()->addMenu(m_TreeBank->WindowMenu());
    menuBar()->addMenu(m_TreeBank->PageMenu());
}

bool MainWindow::IsShaded(){
    return windowOpacity() == 0.0;
}

void MainWindow::Shade(){
    if(!m_TitleBar) return;
    setWindowOpacity(0.0);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
            if(WebView *w = qobject_cast<WebView*>(view->base()))
                w->hide();
            else
#endif
            if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->hide();
        }
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->hide();
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()) receiver->hide();
}

void MainWindow::Unshade(){
    if(!m_TitleBar) return;
    setWindowOpacity(1.0);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
            if(WebView *w = qobject_cast<WebView*>(view->base()))
                w->show();
            else
#endif
            if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
        }
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
    //Receiver *receiver = m_TreeBank->GetReceiver();
    //if(receiver && receiver->IsPurged()) receiver->show();
    SetFocus();
}

void MainWindow::ShowAllEdgeWidgets(){
    m_DialogFrame->show();
    if(!m_TitleBar) return;
    m_TitleBar        ->show();
    m_NorthWidget     ->show();
    m_SouthWidget     ->show();
    m_WestWidget      ->show();
    m_EastWidget      ->show();
    m_NorthWestWidget ->show();
    m_NorthEastWidget ->show();
    m_SouthWestWidget ->show();
    m_SouthEastWidget ->show();
}

void MainWindow::HideAllEdgeWidgets(){
    m_DialogFrame->hide();
    if(!m_TitleBar) return;
    m_TitleBar        ->hide();
    m_NorthWidget     ->hide();
    m_SouthWidget     ->hide();
    m_WestWidget      ->hide();
    m_EastWidget      ->hide();
    m_NorthWestWidget ->hide();
    m_NorthEastWidget ->hide();
    m_SouthWestWidget ->hide();
    m_SouthEastWidget ->hide();
}

void MainWindow::RaiseAllEdgeWidgets(){
    m_DialogFrame->raise();
    if(!m_TitleBar) return;
    m_NorthWidget     ->raise();
    m_SouthWidget     ->raise();
    m_WestWidget      ->raise();
    m_EastWidget      ->raise();

    // for widget's stack order.
    m_TitleBar        ->raise();

    m_NorthWestWidget ->raise();
    m_NorthEastWidget ->raise();
    m_SouthWestWidget ->raise();
    m_SouthEastWidget ->raise();
}

void MainWindow::AdjustAllEdgeWidgets(){
    QRect rect = geometry();
    m_DialogFrame->setGeometry(rect.left(), rect.top(), rect.width(), m_DialogFrame->height());
    m_DialogFrame->Adjust();
    if(!m_TitleBar) return;
    if(IsShaded()) rect.setHeight(0);
    static const int e = EDGE_WIDGET_SIZE;
    static const int t = TITLE_BAR_HEIGHT;
    static const int et = e+t;
    int t0 = isMaximized() ? 0 : t;
    m_TitleBar        ->setGeometry(QRect(rect.left(), rect.top()-t0, rect.width(), t));
    m_NorthWidget     ->setGeometry(QRect(rect.left()-1, rect.top()-et, rect.width()+2, e));
    m_SouthWidget     ->setGeometry(QRect(rect.left()-1, rect.bottom()+1, rect.width()+2, e));
    m_WestWidget      ->setGeometry(QRect(rect.left()-e, rect.top()-t-1, e, rect.height()+t+2));
    m_EastWidget      ->setGeometry(QRect(rect.right()+1, rect.top()-t-1, e, rect.height()+t+2));

    m_NorthWestWidget ->setGeometry(QRect(rect.left()-e, rect.top()-et, e+3, e+3));
    m_NorthEastWidget ->setGeometry(QRect(rect.right()+1-3, rect.top()-et, e+3, e+3));
    m_SouthWestWidget ->setGeometry(QRect(rect.left()-e, rect.bottom()+1-3, e+3, e+3));
    m_SouthEastWidget ->setGeometry(QRect(rect.right()+1-3, rect.bottom()+1-3, e+3, e+3));
}

void MainWindow::UpdateAllEdgeWidgets(){
    if(!m_TitleBar) return;
    m_TitleBar        ->update();
    m_NorthWidget     ->update();
    m_SouthWidget     ->update();
    m_WestWidget      ->update();
    m_EastWidget      ->update();
    m_NorthWestWidget ->update();
    m_NorthEastWidget ->update();
    m_SouthWestWidget ->update();
    m_SouthEastWidget ->update();
}

void MainWindow::SetWindowTitle(const QString &title){
    if(title.isEmpty())
        setWindowTitle(QStringLiteral("vanilla"));
    else if(m_TitleBar)
        setWindowTitle(title);
    else
        setWindowTitle(QStringLiteral("vanilla - ") + title);

    if(m_TitleBar) m_TitleBar->repaint();
}

void MainWindow::ToggleNotifier(){
    m_TreeBank->ToggleNotifier();
}

void MainWindow::ToggleReceiver(){
    m_TreeBank->ToggleReceiver();
}

void MainWindow::ToggleMenuBar(){
    if(IsMenuBarEmpty()){
        CreateMenuBar();
    } else {
        ClearMenuBar();
    }
}

void MainWindow::ToggleFullScreen(){
    if(isFullScreen()){
        showNormal();
    } else {
        showFullScreen();
    }
    SetFocus();
}

void MainWindow::ToggleMaximized(){
    if(isMaximized()){
        showNormal();
    } else {
        showMaximized();
    }
    SetFocus();
}

void MainWindow::ToggleMinimized(){
    if(isMinimized()){
        showNormal();
        SetFocus();
    } else {
        showMinimized();
    }
}

void MainWindow::ToggleShaded(){
    if(IsShaded()){
        Unshade();
        SetFocus();
    } else {
        Shade();
    }
}

void MainWindow::SetMenuBar(bool on){
    if(on && IsMenuBarEmpty()){
        CreateMenuBar();
    } else if(!IsMenuBarEmpty()){
        ClearMenuBar();
    }
}

void MainWindow::SetFullScreen(bool on){
    if(on && !isFullScreen()){
        showFullScreen();
    } else if(isFullScreen()){
        showNormal();
    }
    SetFocus();
}

void MainWindow::SetMaximized(bool on){
    if(on && !isMaximized()){
        showMaximized();
    } else if(isMaximized()){
        showNormal();
    }
    SetFocus();
}

void MainWindow::SetMinimized(bool on){
    if(on && !isMinimized()){
        showMinimized();
    } else if(isMinimized()){
        showNormal();
        SetFocus();
    }
}

void MainWindow::SetShaded(bool on){
    if(on && !IsShaded()){
        Shade();
    } else if(IsShaded()){
        Unshade();
        SetFocus();
    }
}

void MainWindow::closeEvent(QCloseEvent *ev){

    if(Application::GetMainWindows().count() == 1){
        QMainWindow::closeEvent(ev);
        Application::Quit();
    } else {
        Application::RemoveWindow(this);

        foreach(QObject *child, m_TreeBank->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                // 'setParent' of 'QWidget' is not actually performed until calling 'show'.
                view->lower();
                view->show();
                view->hide();
            }
        }
        foreach(QGraphicsItem *item, m_TreeBank->GetScene()->items()){
            if(View *view = dynamic_cast<View*>(item)){
                view->setParent(0);
            }
        }
        foreach(QObject *child, windowHandle()->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                view->hide();
            }
        }
        // segv on destructor of treebank.
        //foreach(View *view, TreeBank::GetAllViews()){
        //    if(view->parent() == m_TreeBank)
        //        view->setParent(0);
        //}

        QMainWindow::closeEvent(ev);
        RemoveSettings();
        TreeBank::SaveSettings();
        Application::SaveGlobalSettings();
        // 'RemoveWindow' doesn't call 'SwitchWindow',
        // but calls 'setFocus' manually.
        deleteLater();
    }
    ev->setAccepted(true);
}

void MainWindow::resizeEvent(QResizeEvent *ev){
    QMainWindow::resizeEvent(ev);
    AdjustAllEdgeWidgets();
}

void MainWindow::moveEvent(QMoveEvent *ev){
    QMainWindow::moveEvent(ev);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
            if(WebView *w = qobject_cast<WebView*>(view->base()))
                w->setGeometry(geometry());
            else
#endif
            if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->setGeometry(geometry());
        }
    }
    AdjustAllEdgeWidgets();
    RaiseAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()){
        notifier->ResizeNotify(m_TreeBank->size());
        notifier->raise();
    }
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()){
        receiver->ResizeNotify(m_TreeBank->size());
        receiver->raise();
    }
}

void MainWindow::showEvent(QShowEvent *ev){
    QMainWindow::showEvent(ev);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
            if(WebView *w = qobject_cast<WebView*>(view->base()))
                w->show();
            else
#endif
            if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
        }
    }
    ShowAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
    //Receiver *receiver = m_TreeBank->GetReceiver();
    //if(receiver && receiver->IsPurged()) receiver->show();
}

void MainWindow::hideEvent(QHideEvent *ev){
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
            if(WebView *w = qobject_cast<WebView*>(view->base()))
                w->show();
            else
#endif
            if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
        }
    }
    HideAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->hide();
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()) receiver->hide();
    QMainWindow::hideEvent(ev);
}

#if defined(Q_OS_WIN)

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result){
    bool ret = QMainWindow::nativeEvent(eventType, message, result);

    if(eventType == "windows_generic_MSG"){
        if(static_cast<MSG*>(message)->message == WM_WINDOWPOSCHANGED){

            if(TreeBank::PurgeView()){
                if(SharedView view = m_TreeBank->GetCurrentView()){
#ifdef QTWEBKIT
                    if(WebView *w = qobject_cast<WebView*>(view->base()))
                        w->raise();
                    else
#endif
                    if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                        w->raise();
                }
            }
            RaiseAllEdgeWidgets();
            Notifier *notifier = m_TreeBank->GetNotifier();
            if(notifier && notifier->IsPurged()) notifier->raise();
            Receiver *receiver = m_TreeBank->GetReceiver();
            if(receiver && receiver->IsPurged()) receiver->raise();
        }
    }
    return ret;
}

#endif // defined(Q_OS_WIN)

namespace{
    static const QPen tpen = QPen(QColor(0,0,0,1));
    static const QPen bpen = QPen(QColor(0,0,0,255));
    static const QBrush tbrush = QBrush(QColor(0,0,0,1));
    static const QBrush bbrush = QBrush(QColor(0,0,0,255));
    static const int none   = 0;
    static const int line   = 1 << 0;
    static const int edge   = 1 << 1;
    static const int shadow = 1 << 2;
    static const int disp = shadow;
}

TitleBar::TitleBar(MainWindow *mainwindow)
    : m_MainWindow(mainwindow)
    , QWidget(0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    m_HoveredButton = NoButton;
    m_Shade = Application::style()->standardIcon(QStyle::SP_TitleBarShadeButton).pixmap(MinimizeAreaRect().size());
    m_Unshade = Application::style()->standardIcon(QStyle::SP_TitleBarUnshadeButton).pixmap(MinimizeAreaRect().size());
    m_Minimize = Application::style()->standardIcon(QStyle::SP_TitleBarMinButton).pixmap(MinimizeAreaRect().size());
    m_Maximize = Application::style()->standardIcon(QStyle::SP_TitleBarMaxButton).pixmap(MaximizeAreaRect().size());
    m_Normal = Application::style()->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(MaximizeAreaRect().size());
    m_Close = Application::style()->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(CloseAreaRect().size());
}

void TitleBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);

    bool isCurrent = m_MainWindow == Application::GetCurrentWindow();

    if(m_MainWindow->isMaximized()){
        painter.setPen(QColor(255,255,255,200));
        painter.setBrush(QColor(255,255,255,isCurrent ? 200 : 128));
        painter.drawRect(QRect(width()-32-28*4 - 6, 0,
                               32+28*4 + 5, size().height()-1));
    } else {
        painter.setPen(QColor(255,255,255,200));
        painter.setBrush(QColor(255,255,255, isCurrent ? 200 : 128));
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));

        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(QColor(0,0,0,255));
        painter.drawRect(MenuAreaRect());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0,0,0,127));
        if(m_HoveredButton == MenuButton) painter.drawRect(MenuAreaRect1());
    }

    if(width() > 19 + 32 + 28*4 + 5){
        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(QColor(0,0,0,255));
        painter.drawRect(ViewTreeAreaRect());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0,0,0,127));
        if(m_HoveredButton == ViewTreeButton) painter.drawRect(ViewTreeAreaRect1());
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0,0,0,127));

    if(width() > 19 + 32 + 28*3 + 5){
        if(m_MainWindow->IsShaded())
            painter.drawPixmap(ShadeAreaRect(), m_Unshade, QRect(QPoint(), m_Unshade.size()));
        else painter.drawPixmap(ShadeAreaRect(), m_Shade, QRect(QPoint(), m_Shade.size()));
        if(m_HoveredButton == ShadeButton) painter.drawRect(ShadeAreaRect1());
    }
    if(width() > 19 + 32 + 28*2 + 5){
        if(m_MainWindow->isMinimized())
            painter.drawPixmap(MinimizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MinimizeAreaRect(), m_Minimize, QRect(QPoint(), m_Minimize.size()));
        if(m_HoveredButton == MinimizeButton) painter.drawRect(MinimizeAreaRect1());
    }
    if(width() > 19 + 32 + 28*1 + 5){
        if(m_MainWindow->isMaximized())
            painter.drawPixmap(MaximizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MaximizeAreaRect(), m_Maximize, QRect(QPoint(), m_Maximize.size()));
        if(m_HoveredButton == MaximizeButton) painter.drawRect(MaximizeAreaRect1());
    }

    painter.drawPixmap(CloseAreaRect(), m_Close, QRect(QPoint(), m_Close.size()));
    if(m_HoveredButton == CloseButton) painter.drawRect(CloseAreaRect1());

    if(!m_MainWindow->isMaximized()){
        painter.setFont(TITLE_BAR_TITLE_FONT);
        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(Qt::NoBrush);
        painter.drawText(QRect(QPoint(22,2), size()-QSize(171,1)),
                         Qt::TextSingleLine,
                         m_MainWindow->windowTitle());
    }
    ev->setAccepted(true);
}

void TitleBar::mousePressEvent(QMouseEvent *ev){
    m_MainWindow->raise();
    m_Moved = false;
    if(ev->button() == Qt::LeftButton){
        m_Pos = ev->globalPos() - geometry().topLeft() - QPoint(0,TITLE_BAR_HEIGHT);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        m_Moved = true;
        if(m_HoveredButton == NoButton)
            m_MainWindow->move(ev->globalPos() - m_Pos);
        return;
    }
    HoveredButton before = m_HoveredButton;
    if(MenuAreaRect1().contains(ev->pos())){
        m_HoveredButton = MenuButton;
    } else if(ViewTreeAreaRect1().contains(ev->pos())){
        m_HoveredButton = ViewTreeButton;
    } else if(ShadeAreaRect1().contains(ev->pos())){
        m_HoveredButton = ShadeButton;
    } else if(MinimizeAreaRect1().contains(ev->pos())){
        m_HoveredButton = MinimizeButton;
    } else if(MaximizeAreaRect1().contains(ev->pos())){
        m_HoveredButton = MaximizeButton;
    } else if(CloseAreaRect1().contains(ev->pos())){
        m_HoveredButton = CloseButton;
    } else {
        m_HoveredButton = NoButton;
    }
    if(before != m_HoveredButton) repaint();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *ev){
    if(m_Moved){
        m_MainWindow->SetFocus();
        m_Moved = false;
        ev->setAccepted(true);
    } else if(ev->button() == Qt::LeftButton){
        if(MenuAreaRect1().contains(ev->pos())){
            QMenu *menu = m_MainWindow->GetTreeBank()->CreateTitlebarMenu();
            menu->exec(ev->globalPos());
            delete menu;
            m_MainWindow->SetFocus();
        } else if(ViewTreeAreaRect1().contains(ev->pos())){
            Gadgets *g = m_MainWindow->GetTreeBank()->GetGadgets();
            if(g && g->IsActive()) g->Deactivate();
            else m_MainWindow->GetTreeBank()->DisplayViewTree();
            m_MainWindow->SetFocus();
        } else if(ShadeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleShaded();
        } else if(MinimizeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleMinimized();
        } else if(MaximizeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleMaximized();
        } else if(CloseAreaRect1().contains(ev->pos())){
            m_MainWindow->close();
        } else {
            m_MainWindow->SetFocus();
        }
    } else if(ev->button() == Qt::RightButton){
        m_MainWindow->ToggleShaded();
    }
    repaint();
    ev->setAccepted(true);
    m_Moved = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *ev){
    if(MenuAreaRect1().contains(ev->pos()) ||
       ViewTreeAreaRect1().contains(ev->pos()) ||
       ShadeAreaRect1().contains(ev->pos()) ||
       MinimizeAreaRect1().contains(ev->pos()) ||
       MaximizeAreaRect1().contains(ev->pos()) ||
       CloseAreaRect1().contains(ev->pos())){
        /* do nothing. */
    } else if(ev->button() == Qt::LeftButton){
        m_MainWindow->ToggleMaximized();
    }
    ev->setAccepted(true);
}

void TitleBar::enterEvent(QEvent *ev){
    Q_UNUSED(ev);
}

void TitleBar::leaveEvent(QEvent *ev){
    Q_UNUSED(ev);
    m_HoveredButton = NoButton;
    repaint();
}

QRect TitleBar::MenuAreaRect(){
    return QRect(5,5,10,10);
}

QRect TitleBar::ViewTreeAreaRect(){
    return QRect(width()-29-28*4,5,10,10);
}

QRect TitleBar::ShadeAreaRect(){
    return QRect(width()-28-28*3,6,10,10);
}

QRect TitleBar::MinimizeAreaRect(){
    return QRect(width()-28-28*2,6,10,10);
}

QRect TitleBar::MaximizeAreaRect(){
    return QRect(width()-28-28*1,6,10,10);
}

QRect TitleBar::CloseAreaRect(){
    return QRect(width()-27-28*0,6,10,10);
}

MainWindowEdgeWidget::MainWindowEdgeWidget(MainWindow *mainwindow)
    : m_MainWindow(mainwindow)
    , QWidget(0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void MainWindowEdgeWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    //painter.setPen(QColor(0,0,0,255));
    //painter.drawLine(QPoint(0,9), QPoint(width(),9));
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mousePressEvent(QMouseEvent *ev){
    m_MainWindow->raise();
    if(ev->button() == Qt::LeftButton)
        m_Pos = ev->pos();
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        QRect rect = m_MainWindow->geometry();
        QPoint pos = mapToGlobal(ev->pos());
        rect = ComputeNewRect(rect, pos);
        m_MainWindow->setGeometry(rect);
    }
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mouseReleaseEvent(QMouseEvent *ev){
    m_MainWindow->SetFocus();
    m_Pos = QPoint();
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::enterEvent(QEvent *ev){
    setCursor(CursorShape());
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::leaveEvent(QEvent *ev){
    setCursor(Qt::ArrowCursor);
    ev->setAccepted(true);
}

QRect MainWindowNorthWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 pos.y() + et - m_Pos.y(),
                 rect.width(),
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowSouthWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 rect.width(),
                 pos.y() - rect.top() - m_Pos.y());
}

QRect MainWindowWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 rect.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 rect.height());
}

QRect MainWindowEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 pos.x() - rect.left() - m_Pos.x(),
                 rect.height());
}

QRect MainWindowNorthWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 pos.y() + et - m_Pos.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowNorthEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 pos.y() + et - m_Pos.y(),
                 pos.x() - rect.left() - m_Pos.x() + 3,
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowSouthWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 rect.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 pos.y() - rect.top() - m_Pos.y() + 3);
}

QRect MainWindowSouthEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 pos.x() - rect.left() - m_Pos.x() + 3,
                 pos.y() - rect.top() - m_Pos.y() + 3);
}

#define EDGE_SHADOW_COLOR qRgba(0,0,0,c)

void MainWindowNorthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            bool isCurrent = m_MainWindow == Application::GetCurrentWindow();
            if(isCurrent){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                for(double i=0; i < width(); i++){
                    for(double j=0; j < height(); j++){
                        double w = width();
                        double c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(i, j, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,9), QPoint(width(),9));
        }
    }
    ev->setAccepted(true);
}

void MainWindowSouthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            bool isCurrent = m_MainWindow == Application::GetCurrentWindow();
            if(isCurrent){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                for(double i=0; i < width(); i++){
                    for(double j=0; j < height(); j++){
                        double w = width();
                        double c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(i, e-j-1, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(width(),0));
        }
    }
    ev->setAccepted(true);
}

void MainWindowWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            bool isCurrent = m_MainWindow == Application::GetCurrentWindow();
            if(isCurrent){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                for(double i=0; i < height(); i++){
                    for(double j=0; j < width(); j++){
                        double w = height();
                        double c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(j, i, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(9,0), QPoint(9,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            bool isCurrent = m_MainWindow == Application::GetCurrentWindow();
            if(isCurrent){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                for(double i=0; i < height(); i++){
                    for(double j=0; j < width(); j++){
                        double w = height();
                        double c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(e-j-1, i, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(0,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowNorthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowNorthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

Qt::CursorShape MainWindowNorthWidget::CursorShape(){ return Qt::SizeVerCursor; }
Qt::CursorShape MainWindowSouthWidget::CursorShape(){ return Qt::SizeVerCursor; }
Qt::CursorShape MainWindowWestWidget::CursorShape(){ return Qt::SizeHorCursor; }
Qt::CursorShape MainWindowEastWidget::CursorShape(){ return Qt::SizeHorCursor; }
Qt::CursorShape MainWindowNorthWestWidget::CursorShape(){ return Qt::SizeFDiagCursor; }
Qt::CursorShape MainWindowNorthEastWidget::CursorShape(){ return Qt::SizeBDiagCursor; }
Qt::CursorShape MainWindowSouthWestWidget::CursorShape(){ return Qt::SizeBDiagCursor; }
Qt::CursorShape MainWindowSouthEastWidget::CursorShape(){ return Qt::SizeFDiagCursor; }
