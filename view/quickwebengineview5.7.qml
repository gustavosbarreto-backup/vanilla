import QtQuick 2.7
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtWebEngine 1.3

Rectangle {
    WebEngineView {
        id: webEngineView
        signal viewChanged()
        signal scrollChanged(point pos)
        signal callBackResult(int id, variant result)

        anchors.fill: parent

        onLoadingChanged: {
            var status = loadRequest.status

            if(status == WebEngineView.LoadStartedStatus){
                viewInterface.loadStarted()
            }
            if(status == WebEngineView.LoadSucceededStatus){
                viewInterface.loadFinished(true)
            }
            if(status == WebEngineView.LoadFaliedStatus){
                viewInterface.loadFinished(false)
            }
        }

        onLoadProgressChanged: {
            viewInterface.loadProgress(loadProgress)
        }

        onLinkHovered: {
            viewInterface.linkHovered(hoveredUrl.toString(), '', '')
        }

        onTitleChanged: {
            viewInterface.titleChanged(title)
        }

        onUrlChanged: {
            viewInterface.urlChanged(url)
        }

        onIconChanged: {
            viewInterface.iconUrlChanged(icon)
        }

        onWindowCloseRequested: {
            viewInterface.windowCloseRequested()
        }

        onJavaScriptConsoleMessage: {
            viewInterface.javascriptConsoleMessage(level, message)
        }

        onFeaturePermissionRequested: {
            viewInterface.featurePermissionRequested(securityOrigin, feature)
        }

        onRenderProcessTerminated: {
            viewInterface.renderProcessTerminated(terminationStatus, exitCode)
        }

        onFullScreenRequested: {
            viewInterface.fullScreenRequested(request.toggleOn)
            request.accept()
        }

        function setScroll(pos){
            runJavaScript
            (viewInterface.setScrollRatioPointJsCode(pos),
             WebEngineScript.MainWorld,
             function(_){
                 emitScrollChangedIfNeed()
             })
        }

        function saveScroll(){
            runJavaScript
            (viewInterface.getScrollValuePointJsCode(),
             WebEngineScript.MainWorld,
             function(result){
                 viewInterface.saveScrollToNode(Qt.point(result[0], result[1]))
             })
        }

        function restoreScroll(){
            var pos = viewInterface.restoreScrollFromNode()
            runJavaScript
            (viewInterface.setScrollValuePointJsCode(pos),
             WebEngineScript.MainWorld)
        }

        function saveZoom(){
            viewInterface.saveZoomToNode(zoomFactor)
        }

        function restoreZoom(){
            zoomFactor = viewInterface.restoreZoomFromNode()
        }

        function evaluateJavaScript(id, code){
            runJavaScript
            (code,
             WebEngineScript.MainWorld,
             function(result){
                 callBackResult(id, result)
             })
        }

        function emitScrollChangedIfNeed(){
            runJavaScript
            (viewInterface.getScrollValuePointJsCode(),
             WebEngineScript.MainWorld,
             function(result){
                 if(Qt.point(result[0], result[1]) ==
                    viewInterface.restoreScrollFromNode())
                     return

                 runJavaScript
                 (viewInterface.getScrollRatioPointJsCode(),
                  WebEngineScript.MainWorld,
                  function(pointf){
                      scrollChanged(Qt.point(pointf[0], pointf[1]))
                  })
             })
        }

        function setFocusToElement(xpath){
            runJavaScript
            (viewInterface.setFocusToElementJsCode(xpath),
             WebEngineScript.MainWorld)
        }

        function fireClickEvent(xpath, pos){
            runJavaScript
            (viewInterface.fireClickEventJsCode(xpath, pos/zoomFactor),
             WebEngineScript.MainWorld)
        }

        function setTextValue(xpath, text){
            runJavaScript
            (viewInterface.setTextValueJsCode(xpath, text),
             WebEngineScript.MainWorld)
        }

        function adjustContents(){
            //experimental.preferredMinimumContentsWidth = parent.width
        }

        function seekText(str, opt){
            var option = 0
            if(opt & viewInterface.findBackwardIntValue())
                option |= FindBackward
            if(opt & viewInterface.caseSensitivelyIntValue())
                option |= FindCaseSensitively

            findText(str, option)
            emitScrollChangedIfNeed()
        }

        function cut(){
            triggerWebAction(WebEngineView.Cut)
        }
        function copy(){
            triggerWebAction(WebEngineView.Copy)
        }
        function paste(){
            triggerWebAction(WebEngineView.Paste)
        }
        function undo(){
            triggerWebAction(WebEngineView.Undo)
        }
        function redo(){
            triggerWebAction(WebEngineView.Redo)
        }
        function selectAll(){
            triggerWebAction(WebEngineView.SelectAll)
        }
        function unselect(){
            runJavaScript("(function(){ document.activeElement.blur(); getSelection().removeAllRanges();})()",
                          WebEngineScript.MainWorld)
        }
        function reloadAndBypassCache(){
            triggerWebAction(WebEngineView.ReloadAndBypassCache)
        }
        function stopAndUnselect(){
            stop(); unselect()
        }
        function print_(){
            // not yet implemented.
        }
        function save(){
            triggerWebAction(WebEngineView.SavePage)
        }

        function grantFeaturePermission_(securityOrigin, feature, granted){
            grantFeaturePermission(securityOrigin, feature, granted);
        }

        function setUserAgent(agent){
            //experimental.userAgent = agent
        }

        function setDefaultTextEncoding(encoding){
            settings.defaultTextEncoding = encoding
        }

        function setPreference(item, value){
            if     (item == "AutoLoadImages")                  settings.autoLoadImages = value
            else if(item == "JavascriptCanAccessClipboard")    settings.javascriptCanAccessClipboard = value
            else if(item == "JavascriptCanOpenWindows")        settings.javascriptCanOpenWindows = value
            else if(item == "JavascriptEnabled")               settings.javascriptEnabled = value
            else if(item == "LinksIncludedInFocusChain")       settings.linksIncludedInFocusChain = value
            else if(item == "LocalContentCanAccessFileUrls")   settings.localContentCanAccessFileUrls = value
            else if(item == "LocalContentCanAccessRemoteUrls") settings.localContentCanAccessRemoteUrls = value
            else if(item == "LocalStorageEnabled")             settings.localStorageEnabled = value
            else if(item == "PluginsEnabled")                  settings.pluginsEnabled = value
            else if(item == "SpatialNavigationEnabled")        settings.spatialNavigationEnabled = value
            else if(item == "HyperlinkAuditingEnabled")        settings.hyperlinkAuditingEnabled = value

            else if(item == "ScreenCaptureEnabled")            settings.screenCaptureEnabled = value
            else if(item == "WebGLEnabled")                    settings.webGLEnabled = value
            else if(item == "Accelerated2dCanvasEnabled")      settings.accelerated2dCanvasEnabled = value
            else if(item == "AutoLoadIconsForPage")            settings.autoLoadIconsForPage = value
            else if(item == "TouchIconsEnabled")               settings.touchIconsEnabled = value

            else if(item == "ErrorPageEnabled")                settings.errorPageEnabled = value
            else if(item == "FullScreenSupportEnabled")        settings.fullScreenSupportEnabled = value
        }

        function setFontFamily(item, value){
            // not implemented yet?
            //if     (item == "StandardFont")  experimental.settings.standardFontFamily = value
            //else if(item == "FixedFont")     experimental.settings.fixedFontFamily = value
            //else if(item == "SerifFont")     experimental.settings.serifFontFamily = value
            //else if(item == "SansSerifFont") experimental.settings.sansSerifFontFamily = value
            //else if(item == "CursiveFont")   experimental.settings.cursiveFontFamily = value
            //else if(item == "FantasyFont")   experimental.settings.fantasyFontFamily = value
        }

        function setFontSize(item, value){
            // not implemented yet?
            //if     (item == "MinimumFontSize")      experimental.settings.minimumFontSize = value
            //else if(item == "DefaultFontSize")      experimental.settings.defaultFontSize = value
            //else if(item == "DefaultFixedFontSize") experimental.settings.defaultFixedFontSize = value
        }
    }
}
