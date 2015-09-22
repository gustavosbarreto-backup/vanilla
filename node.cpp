#include "switch.hpp"
#include "const.hpp"

#ifndef USE_LIGHTNODE

#include "node.hpp"

#include "application.hpp"
#include "treebank.hpp"
#include "view.hpp"

#include <QUuid>

bool Node::m_Booting = false;
QStringList Node::m_AllImageFileName = QStringList();
QStringList Node::m_AllHistoryFileName = QStringList();
bool Node::m_EnableDeepCopyOfNode = false;
Node::AddNodePosition Node::m_AddChildViewNodePosition = RightEnd;
Node::AddNodePosition Node::m_AddSiblingViewNodePosition = RightOfPrimary;

Node::Node()
    : QObject(0)
{
    m_Folded   = true;
    m_View     = 0;
    m_Title    = QString();
    m_Parent   = 0;
    m_Primary  = 0;
    m_Partner  = 0;
    m_Children = NodeList();
}

Node::~Node(){
    //Q_ASSERT(!m_View);

    if(m_View){
        m_View->DeleteLater();
    }
    foreach(Node *nd, m_Children){
        nd->deleteLater();
    }
}

void Node::SetBooting(bool b){
    if(b){
        QDir imageDir = Application::ThumbnailDirectory();
        if(imageDir.exists()){
            m_AllImageFileName = imageDir.entryList();
        } else {
            imageDir.mkpath(Application::ThumbnailDirectory());
        }

        QDir histDir = Application::HistoryDirectory();
        if(histDir.exists()){
            m_AllHistoryFileName = histDir.entryList();
        } else {
            histDir.mkpath(Application::HistoryDirectory());
        }
    } else {
        foreach(QString file, m_AllImageFileName){
            // 'QFile::remove' can't delete directory('.' or '..').
            // when trying to delete, 'remove' returns false.
            QFile::remove(Application::ThumbnailDirectory() + file);
        }
        m_AllImageFileName.clear();

        foreach(QString file, m_AllHistoryFileName){
            // 'QFile::remove' can't delete directory('.' or '..').
            // when trying to delete, 'remove' returns false.
            QFile::remove(Application::HistoryDirectory() + file);
        }
        m_AllHistoryFileName.clear();
    }
    m_Booting = b;
}

void Node::LoadSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("application"));
    m_EnableDeepCopyOfNode = settings->value(QStringLiteral("@EnableDeepCopyOfNode") , false).value<bool>();
    {
        QString position = settings->value(QStringLiteral("@AddChildViewNodePosition"),
                                           QStringLiteral("RightEnd")).value<QString>();
        if(position == QStringLiteral("RightEnd"))                    m_AddChildViewNodePosition = RightEnd;
        if(position == QStringLiteral("LeftEnd"))                     m_AddChildViewNodePosition = LeftEnd;
        if(position == QStringLiteral("RightOfPrimary"))              m_AddChildViewNodePosition = RightOfPrimary;
        if(position == QStringLiteral("LeftOfPrimary"))               m_AddChildViewNodePosition = LeftOfPrimary;
        if(position == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddChildViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(position == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddChildViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
    {
        QString position = settings->value(QStringLiteral("@AddSiblingViewNodePosition"),
                                           QStringLiteral("RightOfPrimary")).value<QString>();
        if(position == QStringLiteral("RightEnd"))                    m_AddSiblingViewNodePosition = RightEnd;
        if(position == QStringLiteral("LeftEnd"))                     m_AddSiblingViewNodePosition = LeftEnd;
        if(position == QStringLiteral("RightOfPrimary"))              m_AddSiblingViewNodePosition = RightOfPrimary;
        if(position == QStringLiteral("LeftOfPrimary"))               m_AddSiblingViewNodePosition = LeftOfPrimary;
        if(position == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddSiblingViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(position == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddSiblingViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
    settings->endGroup();
}

void Node::SaveSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("application"));
    settings->setValue(QStringLiteral("@EnableDeepCopyOfNode"), m_EnableDeepCopyOfNode);
    {
        AddNodePosition position = m_AddChildViewNodePosition;
        if(position == RightEnd)                    settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  settings->setValue(QStringLiteral("@AddChildViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
    {
        AddNodePosition position = m_AddSiblingViewNodePosition;
        if(position == RightEnd)                    settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  settings->setValue(QStringLiteral("@AddSiblingViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
    settings->endGroup();
}

bool Node::IsRead(){
    return GetCreateDate() != GetLastAccessDate();
}

void Node::SetCreateDateToCurrent(){
    SetCreateDate(QDateTime::currentDateTime());
}

void Node::SetLastUpdateDateToCurrent(){
    SetLastUpdateDate(QDateTime::currentDateTime());
}

void Node::SetLastAccessDateToCurrent(){
    SetLastAccessDate(QDateTime::currentDateTime());
}

HistNode::HistNode()
    : Node()
{
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetCreateDate(current);
        SetLastUpdateDate(current);
        SetLastAccessDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
            m_Partner->SetLastAccessDate(current);
        }
    }
    m_Url     = QUrl();
    m_Image   = QImage();
    m_ImageFileName = QString();
    m_NeedToSaveImage = false;
    m_HistoryData = QByteArray();
    m_HistoryFileName = QString();
    m_NeedToSaveHistory = false;
    m_ScrollX = 0;
    m_ScrollY = 0;
    m_Zoom    = 1.0;
    m_Type    = HistTypeNode;
}

HistNode::~HistNode(){
    if(!m_Booting && m_Partner)
        m_Partner->SetLastUpdateDate(QDateTime::currentDateTime());

    // 'GetPartner' and 'SetPartner' is
    // need to be inline method(if virtual),
    // because here is destructor(perhaps...).
    if(m_Partner && m_Partner->GetPartner() == this)
        m_Partner->SetPartner(0);

    //if(view)
    //    view->DeleteLater();
}

bool HistNode::IsRoot(){
    return m_Parent == 0 || m_Parent->GetParent() == 0;
}

bool HistNode::IsDirectory(){
    return false;
}

bool HistNode::IsHistNode(){
    return true;
}

bool HistNode::IsViewNode(){
    return false;
}

bool HistNode::TitleEditable(){
    return false;
}

HistNode *HistNode::MakeChild(){
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }
    HistNode *child = new HistNode();
    child->SetParent(this);
    child->SetZoom(m_Zoom);
    AddChild(child);
    return child;
}

HistNode *HistNode::MakeParent(){
    if(!GetParent()) return 0;

    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }

    GetParent()->RemoveChild(this);
    if(GetParent()->GetPrimary() == this){
        if(GetSibling().length() == 0)
            GetParent()->SetPrimary(0);
        else
            GetParent()->SetPrimary(GetSibling().first());
    }
    HistNode *parent = new HistNode();

    // root doesn't have zoom factor.
    GetParent()->AddChild(parent);
    parent->SetParent(GetParent());
    parent->SetZoom(m_Zoom);
    parent->AddChild(this);
    SetParent(parent);
    return parent;
}

HistNode *HistNode::Next(){
    if(m_Children.isEmpty()) return 0;
    return (m_Primary ? m_Primary : m_Children.last())->ToHistNode();
}

HistNode *HistNode::Prev(){
    if(this->IsRoot()) return 0;
    return m_Parent->ToHistNode();
}

HistNode *HistNode::New(){
    if(m_Booting) return 0;
    HistNode *hn = MakeChild();
    hn->m_Partner = m_Partner;
    hn->m_Url = QUrl(QStringLiteral("about:blank"));
    return hn;
}

HistNode *HistNode::Clone(HistNode *parent, ViewNode *partner){
    if(m_Booting) return 0;
    if(!parent)  parent  = this;
    if(!partner) partner = m_Partner->ToViewNode();
    NodeList children = m_Children; // copy.
    HistNode *clone = parent->MakeChild();
    // Node's member.
    clone->m_Type = m_Type;
    clone->m_Folded = m_Folded;
    clone->m_Title = m_Title;
    clone->m_Partner = partner;
    // set or replace partner.
    if(m_Partner->GetPartner() == this)
        partner->m_Partner = clone;
    // HistNode's member.
    clone->m_Url = m_Url;
    clone->m_Image = QImage(m_Image);
    if(!m_ImageFileName.isEmpty()){
        clone->m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".jpg");
        QFile::copy(Application::ThumbnailDirectory() + m_ImageFileName,
                    Application::ThumbnailDirectory() + clone->m_ImageFileName);
    }
    clone->m_HistoryData = QByteArray(m_HistoryData);
    if(!m_HistoryFileName.isEmpty()){
        clone->m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");
        QFile::copy(Application::HistoryDirectory() + m_HistoryFileName,
                    Application::HistoryDirectory() + clone->m_HistoryFileName);
    }
    clone->m_ScrollX = m_ScrollX;
    clone->m_ScrollY = m_ScrollY;
    clone->m_Zoom = m_Zoom;
    if(m_EnableDeepCopyOfNode){
        // Node::m_Children, Node::m_Primary
        foreach(Node *child, children){
            HistNode *hn = child->ToHistNode()->Clone(clone, partner);
            if(m_Primary == child) clone->m_Primary = hn;
        }
    } else {
        partner->m_Partner = clone;
    }
    return clone;
}

QUrl HistNode::GetUrl(){
    return m_Url;
}

QImage HistNode::GetImage(){
    if(m_Image.isNull() && !m_ImageFileName.isEmpty())
        m_Image = QImage(Application::ThumbnailDirectory() + m_ImageFileName);
    if(m_Image.isNull()) m_ImageFileName = QString();
    return m_Image;
}

int HistNode::GetScrollX(){
    return m_ScrollX;
}

int HistNode::GetScrollY(){
    return m_ScrollY;
}

float HistNode::GetZoom(){
    return m_Zoom;
}

QDateTime HistNode::GetCreateDate(){
    return m_CreateDate;
}

QDateTime HistNode::GetLastUpdateDate(){
    return m_LastUpdateDate;
}

QDateTime HistNode::GetLastAccessDate(){
    return m_LastAccessDate;
}

void HistNode::SetUrl(const QUrl &u){
    if(u != m_Url && !m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }
    m_Url = u;
}

void HistNode::SetImage(const QImage &image){
    if(image.isNull()){
        m_NeedToSaveImage = false;
        if(!m_ImageFileName.isEmpty()){
            QFile::remove(Application::ThumbnailDirectory() + m_ImageFileName);
            m_ImageFileName = QString();
        }
    } else {
        m_NeedToSaveImage = true;
    }
    m_Image = image;
}

void HistNode::SetScrollX(int x){
    m_ScrollX = x;
}

void HistNode::SetScrollY(int y){
    m_ScrollY = y;
}

void HistNode::SetZoom(float z){
    m_Zoom = z;
}

void HistNode::SetCreateDate(QDateTime dt){
    m_CreateDate = dt;
}

void HistNode::SetLastUpdateDate(QDateTime dt){
    m_LastUpdateDate = dt;
}

void HistNode::SetLastAccessDate(QDateTime dt){
    m_LastAccessDate = dt;
}

QString HistNode::GetImageFileName(){
    return m_ImageFileName;
}

void HistNode::SetImageFileName(const QString &s){
    m_ImageFileName = s;
    if(!m_AllImageFileName.isEmpty())
        m_AllImageFileName.removeOne(s);
}

void HistNode::SaveImageIfNeed(){
    if(m_NeedToSaveImage && !m_Image.isNull()){
        if(m_ImageFileName.isEmpty())
            m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".jpg");
        m_Image.save(Application::ThumbnailDirectory() + m_ImageFileName);
        m_NeedToSaveImage = false;
    }
}

QByteArray HistNode::GetHistoryData(){
    if(m_HistoryData.isEmpty() && !m_HistoryFileName.isEmpty() &&
       QFile::exists(Application::HistoryDirectory() + m_HistoryFileName)){
        QFile file(Application::HistoryDirectory() + m_HistoryFileName);
        if(file.open(QIODevice::ReadOnly))
            m_HistoryData = file.readAll();
        file.close();
    }
    if(m_HistoryData.isEmpty()) m_HistoryFileName = QString();
    return m_HistoryData;
}

void HistNode::SetHistoryData(const QByteArray &ba){
    if(ba.isEmpty()){
        m_NeedToSaveHistory = false;
        if(!m_HistoryFileName.isEmpty()){
            QFile::remove(Application::HistoryDirectory() + m_HistoryFileName);
            m_HistoryFileName = QString();
        }
    } else {
        m_NeedToSaveHistory = true;
    }
    m_HistoryData = ba;
}

QString HistNode::GetHistoryFileName(){
    return m_HistoryFileName;
}

void HistNode::SetHistoryFileName(const QString &s){
    m_HistoryFileName = s;
    if(!m_AllHistoryFileName.isEmpty())
        m_AllHistoryFileName.removeOne(s);
}

void HistNode::SaveHistoryIfNeed(){
    if(m_NeedToSaveHistory && !m_HistoryData.isEmpty()){
        if(m_HistoryFileName.isEmpty())
            m_HistoryFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");

        QFile file(Application::HistoryDirectory() + m_HistoryFileName);
        if(file.open(QIODevice::WriteOnly))
            file.write(m_HistoryData);
        file.close();
        m_NeedToSaveHistory = false;
    }
}

ViewNode::ViewNode()
    : Node()
{
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetCreateDate(current);
        SetLastUpdateDate(current);
        SetLastAccessDate(current);
    }
    m_Type = ViewTypeNode;
}

ViewNode::~ViewNode(){
    if(m_Partner){
        Node *nd = m_Partner;

        while(nd->GetParent() && !nd->IsRoot())
            nd = nd->GetParent();

        nd->deleteLater();
    }
}

bool ViewNode::IsRoot(){
    return m_Parent == 0;
}

bool ViewNode::IsDirectory(){
    return !m_Partner;
}

bool ViewNode::IsHistNode(){
    return false;
}

bool ViewNode::IsViewNode(){
    return true;
}

bool ViewNode::TitleEditable(){
    return IsDirectory();
}

ViewNode *ViewNode::MakeChild(bool forceAppend){
    if(!forceAppend) return MakeChild();

    ViewNode *child = new ViewNode();
    child->SetParent(this);
    GetChildren().append(child);
    return child;
}

ViewNode *ViewNode::MakeChild(){
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    ViewNode *child = new ViewNode();
    child->SetParent(this);

    int primaryIndex = m_Primary ? GetChildren().indexOf(m_Primary) : -1;

    if(m_Booting){
        GetChildren().append(child);
        return child;
    }

    switch(m_AddChildViewNodePosition){
    case RightEnd: GetChildren().append(child);  break;
    case LeftEnd:  GetChildren().prepend(child); break;

    case RightOfPrimary:

        if(m_Primary && !GetChildren().isEmpty()){
            GetChildren().insert(primaryIndex+1, child);
        } else {
            GetChildren().append(child);
        }
        break;

    case LeftOfPrimary:

        if(m_Primary && !GetChildren().isEmpty()){
            GetChildren().insert(primaryIndex, child);
        } else {
            GetChildren().prepend(child);
        }
        break;

    case TailOfRightUnreadsOfPrimary:

        if(!m_Primary || GetChildren().isEmpty() ||
           primaryIndex == GetChildren().length()-1){

            GetChildren().append(child);
            break;
        }
        for(int i = primaryIndex+1; i < GetChildren().length(); i++){
            if(i == GetChildren().length()-1){
                GetChildren().append(child);
                break;
            } else if(GetChildren()[i]->IsRead()){
                GetChildren().insert(i, child);
                break;
            }
        }
        break;

    case HeadOfLeftUnreadsOfPrimary:

        if(!m_Primary || GetChildren().isEmpty() ||
           primaryIndex == 0){

            GetChildren().prepend(child);
            break;
        }
        for(int i = primaryIndex-1; i >= 0; i--){
            if(i == 0){
                GetChildren().prepend(child);
                break;
            } else if(GetChildren()[i]->IsRead()){
                GetChildren().insert(i+1, child);
                break;
            }
        }
        break;
    }
    return child;
}

ViewNode *ViewNode::MakeParent(){
    if(!GetParent()) return 0;

    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    GetParent()->RemoveChild(this);
    if(GetParent()->GetPrimary() == this){
        if(GetSibling().length() == 0)
            GetParent()->SetPrimary(0);
        else
            GetParent()->SetPrimary(GetSibling().first());
    }
    ViewNode *parent = new ViewNode();

    GetParent()->AddChild(parent);
    parent->SetParent(GetParent());
    parent->AddChild(this);
    SetParent(parent);
    return parent;
}

ViewNode *ViewNode::MakeSibling(){
    ViewNode *young = new ViewNode();
    young->SetParent(m_Parent);

    int primaryIndex = GetSibling().indexOf(this);

    if(m_Booting){
        GetSibling().insert(primaryIndex+1, young);
        return young;
    }

    switch(m_AddSiblingViewNodePosition){
    case RightEnd: GetSibling().append(young);  break;
    case LeftEnd:  GetSibling().prepend(young); break;
    case RightOfPrimary: GetSibling().insert(primaryIndex+1, young); break;
    case LeftOfPrimary:  GetSibling().insert(primaryIndex, young);   break;

    case TailOfRightUnreadsOfPrimary:

        if(primaryIndex == GetSibling().length()-1){
            GetSibling().append(young);
            break;
        }
        for(int i = primaryIndex+1; i < GetSibling().length(); i++){
            if(GetSibling()[i]->IsRead()){
                GetSibling().insert(i, young);
                break;
            } else if(i == GetSibling().length()-1){
                GetSibling().append(young);
                break;
            }
        }
        break;

    case HeadOfLeftUnreadsOfPrimary:

        if(primaryIndex == 0){
            GetSibling().prepend(young);
            break;
        }
        for(int i = primaryIndex-1; i >= 0; i--){
            if(GetSibling()[i]->IsRead()){
                GetSibling().insert(i+1, young);
                break;
            } else if(i == 0){
                GetSibling().prepend(young);
                break;
            }
        }
        break;
    }
    return young;
}

ViewNode *ViewNode::NewDir(){
    return MakeSibling()->MakeChild();
}

// 'Next' and 'Prev' don't create infinite loop.
// at begin or end of tree, they return 0.

ViewNode *ViewNode::Next(){
    Node *nd = this;
    if(!nd->GetChildren().isEmpty())
        return nd->GetChildren().first()->ToViewNode();
    while(nd->GetParent() && nd->GetSibling().last() == nd)
        nd = nd->GetParent();
    if(nd->IsRoot()) return 0;
    NodeList sibling = nd->GetSibling();
    return sibling[sibling.indexOf(nd) + 1]->ToViewNode();
}

ViewNode *ViewNode::Prev(){
    Node *nd = this;
    if(nd->IsRoot()) return 0;
    if(nd->GetSibling().first() == nd)
        return nd->GetParent()->ToViewNode();
    NodeList sibling = nd->GetSibling();
    nd = sibling[sibling.indexOf(nd) - 1];
    while(!nd->GetChildren().isEmpty())
        nd = nd->GetChildren().last();
    return nd->ToViewNode();
}

ViewNode *ViewNode::New(){
    if(m_Booting) return 0;
    ViewNode *vn = MakeSibling();
    HistNode *hn = TreeBank::GetHistRoot()->MakeChild();
    vn->m_Partner = hn;
    hn->m_Partner = vn;
    hn->m_Url = QUrl(QStringLiteral("about:blank"));
    return vn;
}

ViewNode *ViewNode::Clone(ViewNode *parent){
    if(m_Booting) return 0;
    if(!parent) parent = m_Parent->ToViewNode();
    ViewNode *clone = m_Parent == parent ? MakeSibling() : parent->MakeChild();
    // Node's member.
    clone->m_Type = m_Type;
    clone->m_Folded = m_Folded;
    clone->m_Title = m_Title;

    if(m_EnableDeepCopyOfNode){
        // HistNode::Clone sets ViewNode::m_Partner automatically.
        if(m_Partner){
            // not folder.
            HistNode *root = m_Partner->GetRoot()->ToHistNode();
            root->Clone(TreeBank::GetHistRoot(), clone);
        } else {
            // folder.
            foreach(Node *child, m_Children){
                ViewNode *vn = child->ToViewNode()->Clone(clone);
                if(m_Primary == child) clone->m_Primary = vn;
            }
        }
    } else {
        if(m_Partner){
            // not folder.
            m_Partner->ToHistNode()->Clone(TreeBank::GetHistRoot(), clone);
        }
    }
    return clone;
}

QUrl ViewNode::GetUrl(){
    return m_Partner ? m_Partner->GetUrl() : QUrl();
}

QImage ViewNode::GetImage(){
    return m_Partner ? m_Partner->GetImage() : QImage();
}

int ViewNode::GetScrollX(){
    return m_Partner ? m_Partner->GetScrollX() : 0;
}

int ViewNode::GetScrollY(){
    return m_Partner ? m_Partner->GetScrollY() : 0;
}

float ViewNode::GetZoom(){
    return m_Partner ? m_Partner->GetZoom() : 1.0;
}

QDateTime ViewNode::GetCreateDate(){
    return m_CreateDate;
}

QDateTime ViewNode::GetLastUpdateDate(){
    return m_LastUpdateDate;
}

QDateTime ViewNode::GetLastAccessDate(){
    return m_LastAccessDate;
}

void ViewNode::SetTitle(const QString &title){
    if(!m_Booting && IsDirectory()){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    Node::SetTitle(title);
}

void ViewNode::SetUrl(const QUrl &u){
    if(m_Partner)
        m_Partner->SetUrl(u);
}

void ViewNode::SetImage(const QImage &i){
    if(m_Partner)
        m_Partner->SetImage(i);
}

void ViewNode::SetScrollX(int x){
    if(m_Partner)
        m_Partner->SetScrollX(x);
}

void ViewNode::SetScrollY(int y){
    if(m_Partner)
        m_Partner->SetScrollY(y);
}

void ViewNode::SetZoom(float z){
    if(m_Partner)
        m_Partner->SetZoom(z);
}

void ViewNode::SetCreateDate(QDateTime dt){
    m_CreateDate = dt;
}

void ViewNode::SetLastUpdateDate(QDateTime dt){
    m_LastUpdateDate = dt;
}

void ViewNode::SetLastAccessDate(QDateTime dt){
    m_LastAccessDate = dt;
}

QCache<QString, QImage> LocalNode::m_FileImageCache(DEFAULT_LOCALVIEW_MAX_FILEIMAGE);
QMutex LocalNode::m_DiskAccessMutex(QMutex::NonRecursive);

LocalNode::LocalNode()
    : Node()
{
    m_Url = QUrl();
    m_Type = LocalTypeNode;
    m_Checked = false;
    m_DirFlag = false;
}

LocalNode::~LocalNode(){}

bool LocalNode::IsRoot(){
    return m_Url.toString().endsWith(QStringLiteral(":/")) || m_Url.toString() == QStringLiteral("file:///");
}

bool LocalNode::IsDirectory(){
    if(m_Checked) return m_DirFlag;
    m_Checked = true;
    m_DirFlag = QFileInfo(m_Url.toLocalFile()).isDir();
    return m_DirFlag;
}

bool LocalNode::IsHistNode(){
    return false;
}

bool LocalNode::IsViewNode(){
    return false;
}

bool LocalNode::TitleEditable(){
#ifdef Q_OS_WIN
    return !m_Title.endsWith(QStringLiteral(":/"));
#else
    return true;
#endif
}

void LocalNode::SetTitle(const QString &title){
    if(!m_Title.isEmpty()){
        QString path = m_Url.toLocalFile();
        QStringList list = path.split(QStringLiteral("/"));
        QString name = list.takeLast();

#ifdef Q_OS_WIN
        Q_ASSERT(m_Title.endsWith(QStringLiteral(":/")) || m_Title == name);
#else
        Q_ASSERT(m_Title == name);
#endif

        list << title;
        QString newPath = list.join(QStringLiteral("/"));

        if(!QFile::rename(path, newPath)) return;

        SetUrl(QUrl::fromLocalFile(newPath));
    }
    Node::SetTitle(title);
}

QUrl LocalNode::GetUrl(){
    return m_Url;
}

void LocalNode::SetUrl(const QUrl &u){
    m_Url = u;
}

QDateTime LocalNode::GetCreateDate(){
    if(m_CreateDate.isValid())
        return m_CreateDate;
    return m_CreateDate = QFileInfo(m_Url.toLocalFile()).created();
}

QDateTime LocalNode::GetLastUpdateDate(){
    if(m_LastUpdateDate.isValid())
        return m_LastUpdateDate;
    return m_LastUpdateDate = QFileInfo(m_Url.toLocalFile()).lastModified();
}

QDateTime LocalNode::GetLastAccessDate(){
    if(m_LastAccessDate.isValid())
        return m_LastAccessDate;
    return m_LastAccessDate = QFileInfo(m_Url.toLocalFile()).lastRead();
}

// not instance specific object.
QImage LocalNode::GetImage(){
    if(QImage *i = m_FileImageCache.object(m_Url.toLocalFile()))
        return *i;
    return QImage();
}

#endif //ifndef USE_LIGHTNODE
