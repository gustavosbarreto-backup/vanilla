#include "switch.hpp"
#include "const.hpp"

#include "quickwebengineview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>
#ifdef PASSWORD_MANAGER
# include <QWebEngineProfile>
#endif

#include "view.hpp"
#include "webenginepage.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

#include <memory>

QMap<View*, QUrl> QuickWebEngineView::m_InspectorTable = QMap<View*, QUrl>();

QuickWebEngineView::QuickWebEngineView(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
#if QT_VERSION >= 0x050700
    , QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview5.7.qml")), parent)
#else
    , QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview5.6.qml")), parent)
#endif
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebEngineView = rootObject()->childItems().first();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebEnginePage(nam, this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_Inspector = 0;
    m_PreventScrollRestoration = false;
#ifdef PASSWORD_MANAGER
    m_PreventAuthRegistration = false;
#endif

    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_RequestId = 0;

    connect(m_QmlWebEngineView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebEngineView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));
    connect(m_QmlWebEngineView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));
}

QuickWebEngineView::~QuickWebEngineView(){
    m_InspectorTable.remove(this);
    if(m_Inspector) m_Inspector->deleteLater();
}

void QuickWebEngineView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    if(!page()) return;

    SetPreference(QWebEngineSettings::AutoLoadImages,                    "AutoLoadImages");
    SetPreference(QWebEngineSettings::JavascriptCanAccessClipboard,      "JavascriptCanAccessClipboard");
    SetPreference(QWebEngineSettings::JavascriptCanOpenWindows,          "JavascriptCanOpenWindows");
    SetPreference(QWebEngineSettings::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebEngineSettings::LinksIncludedInFocusChain,         "LinksIncludedInFocusChain");
    SetPreference(QWebEngineSettings::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebEngineSettings::LocalStorageEnabled,               "LocalStorageEnabled");
    SetPreference(QWebEngineSettings::PluginsEnabled,                    "PluginsEnabled");
    SetPreference(QWebEngineSettings::SpatialNavigationEnabled,          "SpatialNavigationEnabled");
    SetPreference(QWebEngineSettings::HyperlinkAuditingEnabled,          "HyperlinkAuditingEnabled");
    SetPreference(QWebEngineSettings::ScrollAnimatorEnabled,             "ScrollAnimatorEnabled");
#if QT_VERSION >= 0x050700
    SetPreference(QWebEngineSettings::ScreenCaptureEnabled,              "ScreenCaptureEnabled");
    SetPreference(QWebEngineSettings::WebAudioEnabled,                   "WebAudioEnabled");
    SetPreference(QWebEngineSettings::WebGLEnabled,                      "WebGLEnabled");
    SetPreference(QWebEngineSettings::Accelerated2dCanvasEnabled,        "Accelerated2dCanvasEnabled");
    SetPreference(QWebEngineSettings::AutoLoadIconsForPage,              "AutoLoadIconsForPage");
    SetPreference(QWebEngineSettings::TouchIconsEnabled,                 "TouchIconsEnabled");
#endif
    SetPreference(QWebEngineSettings::ErrorPageEnabled,                  "ErrorPageEnabled");
    SetPreference(QWebEngineSettings::FullScreenSupportEnabled,          "FullScreenSupportEnabled");

    SetFontFamily(QWebEngineSettings::StandardFont,  "StandardFont");
    SetFontFamily(QWebEngineSettings::FixedFont,     "FixedFont");
    SetFontFamily(QWebEngineSettings::SerifFont,     "SerifFont");
    SetFontFamily(QWebEngineSettings::SansSerifFont, "SansSerifFont");
    SetFontFamily(QWebEngineSettings::CursiveFont,   "CursiveFont");
    SetFontFamily(QWebEngineSettings::FantasyFont,   "FantasyFont");

    SetFontSize(QWebEngineSettings::MinimumFontSize,        "MinimumFontSize");
    SetFontSize(QWebEngineSettings::MinimumLogicalFontSize, "MinimumLogicalFontSize");
    SetFontSize(QWebEngineSettings::DefaultFontSize,        "DefaultFontSize");
    SetFontSize(QWebEngineSettings::DefaultFixedFontSize,   "DefaultFixedFontSize");

    QMetaObject::invokeMethod(m_QmlWebEngineView, "setUserAgent",
                              Q_ARG(QVariant, QVariant::fromValue(page()->userAgentForUrl(QUrl()))));
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setDefaultTextEncoding",
                              Q_ARG(QVariant, QVariant::fromValue(page()->settings()->defaultTextEncoding())));
}

void QuickWebEngineView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(m_QmlWebEngineView, SIGNAL(titleChanged_(const QString&)),
            tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(m_QmlWebEngineView, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(m_QmlWebEngineView, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        connect(this, SIGNAL(ScrollChanged(QPointF)),
                notifier, SLOT(SetScroll(QPointF)));
        connect(notifier, SIGNAL(ScrollRequest(QPointF)),
                this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        connect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                this, SLOT(Load(const QString&)));
        connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                this, SLOT(SeekText(const QString&, View::FindFlags)));
        connect(receiver, SIGNAL(KeyEvent(QString)),
                this, SLOT(KeyEvent(QString)));

        connect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                page(), SLOT(DownloadSuggest(const QUrl&)));
        connect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickWebEngineView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(m_QmlWebEngineView, SIGNAL(titleChanged_(const QString&)),
               tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(m_QmlWebEngineView, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(m_QmlWebEngineView, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                   notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        disconnect(this, SIGNAL(ScrollChanged(QPointF)),
                   notifier, SLOT(SetScroll(QPointF)));
        disconnect(notifier, SIGNAL(ScrollRequest(QPointF)),
                   this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        disconnect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                   this, SLOT(Load(const QString&)));
        disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                   this, SLOT(SeekText(const QString&, View::FindFlags)));
        disconnect(receiver, SIGNAL(KeyEvent(QString)),
                   this, SLOT(KeyEvent(QString)));

        disconnect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                   page(), SLOT(DownloadSuggest(const QUrl&)));
        disconnect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                   receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickWebEngineView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlWebEngineView->setProperty("devicePixelRatio", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebEngineView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlWebEngineView->setProperty("devicePixelRatio", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

QAction *QuickWebEngineView::Action(QWebEnginePage::WebAction a){
    switch(a){
    case QWebEnginePage::Reload:  return Action(Page::_Reload);
    case QWebEnginePage::Stop:    return Action(Page::_Stop);
    case QWebEnginePage::Back:    return Action(Page::_Back);
    case QWebEnginePage::Forward: return Action(Page::_Forward);
    }
    return page()->Action(a);
}

QAction *QuickWebEngineView::Action(Page::CustomAction a, QVariant data){
    QAction *action = m_ActionTable[a];

    if(action) return action;

    if(a == Page::_Reload ||
       a == Page::_Stop ||
       a == Page::_Back ||
       a == Page::_Forward){

        m_ActionTable[a] = action = new QAction(this);

        switch(a){
        case Page::_Reload:
            action->setText(tr("Reload"));
            connect(action, SIGNAL(triggered()), m_QmlWebEngineView, SLOT(reload()));
            break;
        case Page::_Stop:
            action->setText(tr("Stop"));
            connect(action, SIGNAL(triggered()), m_QmlWebEngineView, SLOT(stop()));
            break;
        case Page::_Back:
            action->setText(tr("Back"));
            connect(action, SIGNAL(triggered()), m_QmlWebEngineView, SLOT(goBack()));
            break;
        case Page::_Forward:
            action->setText(tr("Forward"));
            connect(action, SIGNAL(triggered()), m_QmlWebEngineView, SLOT(goForward()));
            break;
        }
        action->setData(data);
        return action;
    }
    return page()->Action(a, data);
}

void QuickWebEngineView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;
    AssignInspector();
#ifdef USE_WEBCHANNEL
    //page()->AddJsObject();
#endif

#if QT_VERSION < 0x050700
    //if(m_Icon.isNull() && url() != QUrl(QStringLiteral("about:blank")))
    //    UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
#endif
}

void QuickWebEngineView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickWebEngineView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

    AssignInspector();

#ifdef PASSWORD_MANAGER
    QString data = Application::GetAuthDataWithNoDialog
        (page()->profile()->storageName() +
         QStringLiteral(":") + url().host());

    if(!data.isEmpty()){
        CallWithEvaluatedJavaScriptResult(DecorateFormFieldJsCode(data),
                                          [](QVariant){});
    }

    CallWithEvaluatedJavaScriptResult(QStringLiteral(
        "(function(){\n"
      VV"    var forms = document.querySelectorAll(\"form\");\n"
      VV"    for(var i = 0; i < forms.length; i++){\n"
      VV"        var form = forms[i];\n"
      VV"        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
      VV"        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
      VV"        if(!submit || !password) continue;\n"
      VV"        form.addEventListener(\"submit\", function(e){\n"
      VV"            var data = \"\";\n"
      VV"            var inputs = e.target.querySelectorAll(\"input,textarea\");\n"
      VV"            for(var j = 0; j < inputs.length; j++){\n"
      VV"                var field = inputs[j];\n"
      VV"                var type = (field.type || \"hidden\").toLowerCase();\n"
      VV"                var name = field.name;\n"
      VV"                var val = field.value;\n"
      VV"                if(!name || type == \"hidden\" || type == \"submit\"){\n"
      VV"                    continue;\n"
      VV"                }\n"
      VV"                if(data) data = data + \"&\";\n"
      VV"                data = data + encodeURIComponent(name) + \"=\" + encodeURIComponent(val);\n"
      VV"            }\n"
      VV"            console.info(\"submit%1,\" + data);\n"
      VV"        }, false);\n"
      VV"    }\n"
      VV"})()").arg(Application::EventKey()), [](QVariant){});
#endif //ifdef PASSWORD_MANAGER

    static const QList<QEvent::Type> types =
        QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease;

    CallWithEvaluatedJavaScriptResult(InstallEventFilterJsCode(types),
                                      [](QVariant){});

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void QuickWebEngineView::OnTitleChanged(const QString &title){
    Q_UNUSED(title);
}

void QuickWebEngineView::OnUrlChanged(const QUrl &url){
    Q_UNUSED(url);
}

void QuickWebEngineView::OnViewChanged(){
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickWebEngineView::OnScrollChanged(){
    SaveScroll();
}

void QuickWebEngineView::CallWithScroll(PointFCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toPointF());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetScrollRatioPointJsCode())));
}

void QuickWebEngineView::SetScrollBarState(){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    if(!result.isValid()) return;
                    QVariantList list = result.toList();
                    int hmax = list[0].toInt();
                    int vmax = list[1].toInt();
                    if(hmax < 0) hmax = 0;
                    if(vmax < 0) vmax = 0;
                    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
                    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
                    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
                    else             m_ScrollBarState = NoScrollBarEnabled;
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetScrollBarStateJsCode())));
}

void QuickWebEngineView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickWebEngineView::SeekText(const QString &str, View::FindFlags opt){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "seekText",
                              Q_ARG(QVariant, QVariant::fromValue(str)),
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    return true;
}

void QuickWebEngineView::AssignInspector(){
    if(m_InspectorTable.contains(this)) return;

    QString addr = QStringLiteral("http://localhost:%1").arg(Application::RemoteDebuggingPort());
    QNetworkRequest req(QUrl(addr + QStringLiteral("/json")));
    DownloadItem *item = NetworkController::Download
        (static_cast<NetworkAccessManager*>(page()->networkAccessManager()),
         req, NetworkController::ToVariable);

    if(!item) return;

    item->setParent(base());

    connect(item, &DownloadItem::DownloadResult, [this, addr](const QByteArray &result){

            foreach(QJsonValue value, QJsonDocument::fromJson(result).array()){

                QString debuggeeValue = value.toObject()["url"].toString();
                QString debuggerValue = value.toObject()["devtoolsFrontendUrl"].toString();

                if(debuggeeValue.isEmpty() || debuggerValue.isEmpty()) break;

                QUrl debuggee = QUrl(debuggeeValue);
                QUrl debugger = QUrl(addr + debuggerValue);

                if(url() == debuggee && !m_InspectorTable.values().contains(debugger)){
                    m_InspectorTable[this] = debugger;
                    break;
                }
            }
        });
}

void QuickWebEngineView::handleJavascriptConsoleMessage(QString msg){
#ifdef PASSWORD_MANAGER
    static QString reg;
    if(reg.isEmpty()) reg = QStringLiteral("submit%1,([^,]+)").arg(Application::EventKey());

    if(!page()->profile()->isOffTheRecord() &&
       !m_PreventAuthRegistration &&
       Application::ExactMatch(reg, msg)){

        Application::RegisterAuthData
            (page()->profile()->storageName() +
             QStringLiteral(":") + url().host(),
             msg.split(QStringLiteral(","))[1]);
        return;
    }
#endif //ifdef PASSWORD_MANAGER
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        KeyPressEvent(&QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers));
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        KeyReleaseEvent(&QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers));
    } /*else if(Application::ExactMatch(QStringLiteral("mouseMoveEvent%1,([0-9]+),([0-9]+),([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[6] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        Qt::MouseButton button = Qt::NoButton;
        switch(args[1].toInt()){
        case 0: button = Qt::LeftButton;  break;
        case 1: button = Qt::MidButton;   break;
        case 2: button = Qt::RightButton; break; }
        MouseMoveEvent(&QMouseEvent(QEvent::MouseMove, QPointF(args[2].toInt(), args[3].toInt()), button, button, modifiers));
    } else if(Application::ExactMatch(QStringLiteral("mousePressEvent%1,([0-9]+),([0-9]+),([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[6] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        Qt::MouseButton button = Qt::NoButton;
        switch(args[1].toInt()){
        case 0: button = Qt::LeftButton;  break;
        case 1: button = Qt::MidButton;   break;
        case 2: button = Qt::RightButton; break; }
        MousePressEvent(&QMouseEvent(QEvent::MouseButtonPress, QPointF(args[2].toInt(), args[3].toInt()), button, button, modifiers));
    } else if(Application::ExactMatch(QStringLiteral("mouseReleaseEvent%1,([0-9]+),([0-9]+),([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[6] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        Qt::MouseButton button = Qt::NoButton;
        switch(args[1].toInt()){
        case 0: button = Qt::LeftButton;  break;
        case 1: button = Qt::MidButton;   break;
        case 2: button = Qt::RightButton; break; }
        MouseReleaseEvent(&QMouseEvent(QEvent::MouseButtonRelease, QPointF(args[2].toInt(), args[3].toInt()), button, button, modifiers));
    } else if(Application::ExactMatch(QStringLiteral("wheelEvent%1,(-?[0-9]+)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        WheelEvent(&QWheelEvent(QPointF(), args[1].toInt(), Qt::NoButton, Qt::NoModifier, Qt::Vertical));
    }
    */
}

void QuickWebEngineView::hideEvent(QHideEvent *ev){
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickWebEngineView::showEvent(QShowEvent *ev){
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickWebEngineView::keyPressEvent(QKeyEvent *ev){
    if(!visible()) return;

#ifdef PASSWORD_MANAGER
    if(ev->modifiers() & Qt::ControlModifier &&
       ev->key() == Qt::Key_Return){

        QString data = Application::GetAuthData
            (page()->profile()->storageName() +
             QStringLiteral(":") + url().host());

        if(!data.isEmpty()){
            m_PreventAuthRegistration = true;
            CallWithEvaluatedJavaScriptResult
                (View::SubmitFormDataJsCode(data),
                 [this](QVariant){
                    m_PreventAuthRegistration = false;
                });
            ev->setAccepted(true);
            return;
        }
    }
#endif //ifdef PASSWORD_MANAGER

    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    //QQuickWidget::keyPressEvent(ev);

    int k = ev->key();
    if(!m_PreventScrollRestoration &&
       (k == Qt::Key_Space ||
        k == Qt::Key_Up ||
        k == Qt::Key_Down ||
        k == Qt::Key_Right ||
        k == Qt::Key_Left ||
        k == Qt::Key_PageUp ||
        k == Qt::Key_PageDown ||
        k == Qt::Key_Home ||
        k == Qt::Key_End)){

        m_PreventScrollRestoration = true;
    }

    if(/*!ev->isAccepted() &&*/
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickWebEngineView::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev);

    if(!visible()) return;

    //QQuickWidget::keyReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
}

void QuickWebEngineView::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "adjustContents");
}

void QuickWebEngineView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_RightGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_RightGestureMap[gesture])
              ? Action(Page::StringToAction(m_RightGestureMap[gesture]))->text()
            : m_RightGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
        ev->setAccepted(false);
        return;
    }

    int scrollBarWidth = Application::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    bool horizontal = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == HorizontalScrollBarEnabled;
    bool vertical   = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == VerticalScrollBarEnabled;
    QRect touchableRect =
        QRect(QPoint(),
              size() - QSize(vertical   ? scrollBarWidth : 0,
                             horizontal ? scrollBarWidth : 0));

    if(ev->buttons() & Qt::LeftButton &&
       !m_GestureStartedPos.isNull() &&
       touchableRect.contains(m_GestureStartedPos) &&
       (m_ClickedElement &&
        !m_ClickedElement->IsNull() &&
        !m_ClickedElement->IsEditableElement())){

        if(QLineF(ev->pos(), m_GestureStartedPos).length() < 2){
            // gesture not aborted.
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QDrag *drag = new QDrag(this);

        // clear or make directory if need.
        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            // call default behavior.
            GestureAborted();
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = ev->pos() - rect.topLeft();

        if(pixmap.size().width()  > MAX_DRAGGING_PIXMAP_WIDTH ||
           pixmap.size().height() > MAX_DRAGGING_PIXMAP_HEIGHT){

            pos /= qMax(static_cast<float>(pixmap.size().width()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_WIDTH),
                        static_cast<float>(pixmap.size().height()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_HEIGHT));
            pixmap = pixmap.scaled(MAX_DRAGGING_PIXMAP_WIDTH,
                                   MAX_DRAGGING_PIXMAP_HEIGHT,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }

        if(m_HadSelection){
            mime->setImageData(pixmap.toImage());
        } else {
            QPixmap element = m_ClickedElement->Pixmap();
            if(element.isNull())
                mime->setImageData(pixmap.toImage());
            else
                mime->setImageData(element.toImage());
        }
        if(m_EnableDragHackLocal){
            GestureMoved(ev->pos());
        } else {
            GestureAborted();
        }
        m_DragStarted = true;
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(pos);
        drag->exec(Qt::CopyAction | Qt::MoveAction);
        drag->deleteLater();
        ev->setAccepted(true);
    } else {
        // call default behavior.
        GestureAborted();
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void QuickWebEngineView::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos());
    QQuickWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::keyboardModifiers() & Qt::ShiftModifier ||
           Application::keyboardModifiers() & Qt::ControlModifier ||
           ev->button() == Qt::MidButton){

            GestureAborted();
            m_TreeBank->OpenInNewViewNode(req, Page::Activate(), GetViewNode());
            ev->setAccepted(true);
            return;

        } else if(
            // it's requirements of starting loadhack.
            // loadhack uses new hist node instead of same view's `load()'.
            m_EnableLoadHackLocal
            // url is not empty.
            && !url().isEmpty()
            // link doesn't hold jump command.
            && !link.toEncoded().contains("#")
            // m_ClickedElement doesn't hold javascript function.
            && !m_ClickedElement->IsJsCommandElement()){

            GestureAborted();
            m_TreeBank->OpenInNewHistNode(req, true, GetHistNode());
            ev->setAccepted(true);
            return;
        }
    }

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickWebEngineView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QQuickWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    QPoint pos = ev->pos();
    QList<QUrl> urls = ev->mimeData()->urls();
    QObject *source = ev->source();
    QWidget *widget = this;
    QString text;
    if(!ev->mimeData()->text().isEmpty()){
        text = ev->mimeData()->text().replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    CallWithHitElement(pos, [this, pos, urls, text, source, widget](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(urls.isEmpty() || source == widget){
        // do nothing.
    } else if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source)){
        QList<QUrl> filtered;
        foreach(QUrl u, urls){ if(!u.isLocalFile()) filtered << u;}
        m_TreeBank->OpenInNewViewNode(filtered, true, GetViewNode());
        return;
    } else {
        // foreign drag.
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    QQuickWidget::dropEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickWebEngineView::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

    QString wheel;
    bool up = ev->delta() > 0;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){
            View::TriggerAction(str, ev->pos());
        }
        ev->setAccepted(true);

    } else {
        QQuickWidget::wheelEvent(ev);
        ev->setAccepted(true);
    }
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
}

void QuickWebEngineView::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickWebEngineView::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickWebEngineView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickWebEngineView::CallWithGotBaseUrl(UrlCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetBaseUrlJsCode())));
}

void QuickWebEngineView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    // this implementation is same as baseurl...
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetBaseUrlJsCode())));
}

void QuickWebEngineView::CallWithFoundElements(Page::FindElementsOption option,
                                             WebElementListCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    QVariantList list = var.toMap().values();
                    SharedWebElementList result;

                    MainWindow *win = Application::GetCurrentWindow();
                    QSize s =
                        m_TreeBank ? m_TreeBank->size() :
                        win ? win->GetTreeBank()->size() :
                        !size().isEmpty() ? size() :
                        DEFAULT_WINDOW_SIZE;
                    QRect viewport = QRect(QPoint(), s);

                    for(int i = 0; i < list.length(); i++){
                        std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                        *e = JsWebElement(this, list[i]);
                        if(!viewport.intersects(e->Rectangle()))
                            e->SetRectangle(QRect());
                        result << e;
                    }
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(FindElementsJsCode(option))));
}

void QuickWebEngineView::CallWithHitElement(const QPoint &pos,
                                          WebElementCallBack callBack){
    if(pos.isNull()){
        callBack(SharedWebElement());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                    *e = JsWebElement(this, var);
                    callBack(e);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitElementJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebEngineView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()){
        callBack(QUrl());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitLinkUrlJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebEngineView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()){
        callBack(QUrl());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitImageUrlJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebEngineView::CallWithSelectedText(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectedTextJsCode())));
}

void QuickWebEngineView::CallWithSelectedHtml(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectedHtmlJsCode())));
}

void QuickWebEngineView::CallWithWholeText(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(WholeTextJsCode())));
}

void QuickWebEngineView::CallWithWholeHtml(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(WholeHtmlJsCode())));
}

void QuickWebEngineView::CallWithSelectionRegion(RegionCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    QRegion region;
                    if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap)){
                        callBack(region);
                        return;
                    }
                    QVariantMap map = var.toMap();
                    QRect viewport = QRect(QPoint(), size());
                    foreach(QString key, map.keys()){
                        QVariantMap m = map[key].toMap();
                        region |= QRect(m["x"].toInt(),
                                        m["y"].toInt(),
                                        m["width"].toInt(),
                                        m["height"].toInt()).intersected(viewport);
                    }
                    callBack(region);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectionRegionJsCode())));
}

void QuickWebEngineView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                         VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

