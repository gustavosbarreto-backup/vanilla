#include "switch.hpp"
#include "const.hpp"

#include "gadgetsstyle.hpp"

#include <QPainter>
#include <QGraphicsScene>
#include <QCursor>
#include <QGraphicsDropShadowEffect>

#include "graphicstableview.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "gadgets.hpp"
#include "accessiblewebelement.hpp"

QFont GlassStyle::m_ThumbnailTitleFont = QFont(DEFAULT_FONT, 10);

int GlassStyle::m_ThumbnailPaddingX = 2;
int GlassStyle::m_ThumbnailPaddingY = 2;
int GlassStyle::m_ThumbnailTitleHeight = 20;
int GlassStyle::m_ThumbnailWidthPercentage = 15;
int GlassStyle::m_ThumbnailDefaultColumnCount = 4;
int GlassStyle::m_ThumbnailAreaWidthPercentage =
    GlassStyle::m_ThumbnailWidthPercentage *
    GlassStyle::m_ThumbnailDefaultColumnCount;

bool GlassStyle::m_ThumbnailDrawBorder = false;

QSize GlassStyle::m_DefaultThumbnailWholeSize =
    QSize(DEFAULT_THUMBNAIL_SIZE.width()  + GlassStyle::m_ThumbnailPaddingX * 2,
          DEFAULT_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2 + GlassStyle::m_ThumbnailTitleHeight);

QSize GlassStyle::m_MinimumThumbnailWholeSize =
    QSize(MINIMUM_THUMBNAIL_SIZE.width()  + GlassStyle::m_ThumbnailPaddingX * 2,
          MINIMUM_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2 + GlassStyle::m_ThumbnailTitleHeight);;

QFont GlassStyle::m_NodeTitleFont = QFont(DEFAULT_FONT, 13);

int GlassStyle::m_NodeTitleHeight = 20;
bool GlassStyle::m_NodeTitleDrawBorder = false;

int GlassStyle::m_InPlaceNotifierWidth =
    500 + DEFAULT_THUMBNAIL_SIZE.width() + GlassStyle::m_ThumbnailPaddingX * 3;
int GlassStyle::m_InPlaceNotifierHeight =
    DEFAULT_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2;
bool GlassStyle::m_InPlaceNotifierDrawBorder = false;

void GlassStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight){
    const float zoom = gtv->m_CurrentThumbnailZoomFactor; // alias

    int wholeWidth = gtv->m_Size.width() - DISPLAY_PADDING_X * 2;
    int areaWidth = wholeWidth * m_ThumbnailAreaWidthPercentage / 100;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = m_MinimumThumbnailWholeSize.width();
    int defWidth = m_DefaultThumbnailWholeSize.width() * zoom;

    if(col < 1) col = 1;

    if(defWidth < minWidth) defWidth = minWidth;

    if(thumbWidth > defWidth){
        col += (areaWidth - (defWidth * col)) / defWidth;
        thumbWidth = defWidth;
    }

    if(thumbWidth < minWidth){
        col -= ((minWidth - thumbWidth) * col) / minWidth;
        if(col > 1) col -= 1;
        if(col == 0) col = 1;
        thumbWidth = minWidth;
    }

    // compute height from width.
    const int marginWidth  = m_DefaultThumbnailWholeSize.width()  - DEFAULT_THUMBNAIL_SIZE.width();
    const int marginHeight = m_DefaultThumbnailWholeSize.height() - DEFAULT_THUMBNAIL_SIZE.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - DISPLAY_PADDING_Y) / thumbHeight;

    if(line < 1) line = 1;
}

void GlassStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter){
    QColor black(0, 0, 0, gtv->IsDisplayingNode() ? 170 : 100);
    QBrush brush(black);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
}

void GlassStyle::Render(Thumbnail *thumb, QPainter *painter){
    if(!thumb->GetNode() ||
       !thumb->boundingRect().translated(thumb->pos()).intersects(thumb->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    const QRectF bound = thumb->boundingRect();
    const QSize size = bound.size().toSize();
    const QRectF rect = QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QImage image = thumb->GetNode()->GetImage();
    const QString title = thumb->GetNode()->GetTitle();
    const QUrl url = thumb->GetNode()->GetUrl();

    if(image.isNull()){
        Node *tempnode = thumb->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    if(thumb->GetNode()->GetView()){
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,200,0));
        hasviewgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,200,44));
        const QBrush hasviewbrush(hasviewgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hasviewbrush);
        painter->drawRect(rect);
    }

    if(thumb->IsPrimary()){
        QLinearGradient primarygrad;
        primarygrad.setStart(0, start);
        primarygrad.setFinalStop(0, stop);
        primarygrad.setColorAt(static_cast<qreal>(0), QColor(0,100,255,0));
        primarygrad.setColorAt(static_cast<qreal>(1), QColor(0,100,255,77));
        const QBrush primarybrush(primarygrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(primarybrush);
        painter->drawRect(rect);
    }

    if(thumb->IsHovered()){
        QLinearGradient hoveredgrad;
        hoveredgrad.setStart(0, start);
        hoveredgrad.setFinalStop(0, stop);
        hoveredgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,255,0));
        hoveredgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,255,77));
        const QBrush hoveredbrush(hoveredgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hoveredbrush);
        painter->drawRect(rect);
    }

    if(thumb->isSelected()){
        QLinearGradient selectedgrad;
        selectedgrad.setStart(0, start);
        selectedgrad.setFinalStop(0, stop);
        selectedgrad.setColorAt(static_cast<qreal>(0), QColor(255,200,220,0));
        selectedgrad.setColorAt(static_cast<qreal>(1), QColor(255,200,220,170));
        const QBrush selectedbrush(selectedgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedbrush);
        painter->drawRect(rect);
    }

    if(m_ThumbnailDrawBorder){
        painter->setPen(QColor(255,255,255,255));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    QRect image_rect =
        QRect(bound.topLeft().toPoint() +
              QPoint(m_ThumbnailPaddingX,
                     m_ThumbnailPaddingY),
              QSize(size.width()
                    - (m_ThumbnailPaddingX * 2),
                    size.height()
                    - (m_ThumbnailPaddingY * 2
                       + m_ThumbnailTitleHeight))
              - QSize(1,1));

    QRect title_rect =
        QRect(bound.topLeft().toPoint() +
              QPoint(m_ThumbnailPaddingX,
                     size.height()
                     - m_ThumbnailPaddingY
                     - m_ThumbnailTitleHeight),
              QSize(size.width()
                    - (m_ThumbnailPaddingX * 2),
                    m_ThumbnailTitleHeight));

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        painter->drawImage(QRect(image_rect.topLeft()
                                 + QPoint(width_diff/2, height_diff/2),
                                 size),
                           image,
                           QRect(QPoint(), image.size()));
    } else if(thumb->GetNode()->IsDirectory()){
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 100, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 120, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    painter->setFont(m_ThumbnailTitleFont);
    painter->setPen(QPen(QColor(255,255,255,255)));
    painter->setBrush(Qt::NoBrush);

    if(!title.isEmpty()){
        painter->drawText(title_rect, Qt::AlignLeft,
                          thumb->GetNode()->IsDirectory() ?
                          QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first() : title);
    } else {
        painter->drawText(title_rect, Qt::AlignLeft,
                          !url.isEmpty() ? url.toString() :
                          thumb->GetNode()->IsDirectory() ? QStringLiteral("Directory") : QStringLiteral("No Title"));
    }
    painter->restore();
}

void GlassStyle::Render(NodeTitle *title, QPainter *painter){
    QRectF bound = title->boundingRect();
    QSize size = bound.size().toSize();
    QRectF rect = title->GetTableView()->NodeTitleAreaRect();

    if(!title->GetNode() || !rect.isValid() ||
       !bound.translated(title->pos()).intersects(title->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    rect = m_NodeTitleDrawBorder ?
        rect.intersected(QRectF(bound.topLeft(), bound.size() - QSizeF(1,1))):
        QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QString title_ = title->GetNode()->GetTitle();
    const QUrl url = title->GetNode()->GetUrl();

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    if(title->GetNode()->GetView()){
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,200,0));
        hasviewgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,200,44));
        const QBrush hasviewbrush(hasviewgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hasviewbrush);
        painter->drawRect(rect);
    }

    if(title->IsPrimary()){
        QLinearGradient primarygrad;
        primarygrad.setStart(0, start);
        primarygrad.setFinalStop(0, stop);
        primarygrad.setColorAt(static_cast<qreal>(0), QColor(0,100,255,0));
        primarygrad.setColorAt(static_cast<qreal>(1), QColor(0,100,255,77));
        const QBrush primarybrush(primarygrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(primarybrush);
        painter->drawRect(rect);
    }

    if(title->IsHovered()){
        QLinearGradient hoveredgrad;
        hoveredgrad.setStart(0, start);
        hoveredgrad.setFinalStop(0, stop);
        hoveredgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,255,0));
        hoveredgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,255,77));
        const QBrush hoveredbrush(hoveredgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hoveredbrush);
        painter->drawRect(rect);
    }

    if(title->isSelected()){
        QLinearGradient selectedgrad;
        selectedgrad.setStart(0, start);
        selectedgrad.setFinalStop(0, stop);
        selectedgrad.setColorAt(static_cast<qreal>(0), QColor(255,200,220,0));
        selectedgrad.setColorAt(static_cast<qreal>(1), QColor(255,200,220,170));
        const QBrush selectedbrush(selectedgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedbrush);
        painter->drawRect(rect);
    }

    painter->setFont(m_NodeTitleFont);
    painter->setPen(QPen(QColor(255,255,255,255)));
    painter->setBrush(Qt::NoBrush);

    QRectF title_rect = rect.
        intersected(QRectF(bound.topLeft() +
                           QPointF(title->GetNest() * 20 + 5, 0),
                           QSizeF(size.width()-5, m_NodeTitleHeight)));

    if(title_.isEmpty())
        if(url.isEmpty())
            title_ = title->GetNode()->IsDirectory() ? QStringLiteral("Directory") : QStringLiteral("No Title");
        else
            title_ = url.toString();

    const QString prefix = title->GetTableView()->GetDirectoryPrefix(title->GetNode());

    if(!prefix.isEmpty()){
        painter->drawText(title_rect, Qt::AlignLeft, prefix);
        title_rect = QRect(title_rect.x() + 15,
                           title_rect.y(),
                           title_rect.width() - 15,
                           title_rect.height());
    }
    painter->drawText(title_rect, Qt::AlignLeft,
                      title->GetNode()->IsDirectory() ?
                      QStringLiteral("Dir - ") + title_.split(QStringLiteral(";")).first() : title_);
    painter->restore();
}

void GlassStyle::Render(SpotLight *light, QPainter *painter){
    const GraphicsTableView* parent = static_cast<GraphicsTableView*>(light->parentItem());

    const bool p = light->GetType() == GraphicsTableView::PrimarySpotLight;
    const bool h = light->GetType() == GraphicsTableView::HoveredSpotLight;
    const bool l = light->GetType() == GraphicsTableView::LoadedSpotLight;

    const int index =
        p ? parent->m_PrimaryItemIndex :
        h ? parent->m_HoveredItemIndex :
        l ? light->GetIndex() : -1; // default : -1

    if(index == -1 ||
       index >= parent->m_DisplayThumbnails.length())
        return;

    painter->save();

    const QSize size = parent->m_Size.toSize() + QSize(1, 1);

    const Thumbnail *thumb = parent->m_DisplayThumbnails[index];
    const NodeTitle *title = parent->m_DisplayNodeTitles[index];

    const int x1  = thumb->pos().x() + thumb->rect().right();
    const int x2  = title->pos().x() + title->rect().left();

    const int y1b = thumb->pos().y() + thumb->rect().top();
    const int y1e = thumb->pos().y() + thumb->rect().bottom();

    const int y2b = title->pos().y() + title->rect().top();
    const int y2e = title->pos().y() + title->rect().bottom();

    const int ybrange = y2b - y1b;
    const int yerange = y2e - y1e;

    const double xrange = x2 - x1;
    const int begx = qMin(x1, x2);
    const int endx = qMax(x1, x2);

    double yrange, begy, endy;
    double progress;
    int x, y;

    const QRectF bound = light->boundingRect();
    const QRectF rect = QRectF(bound.topLeft(), bound.size());

    QImage image(size, QImage::Format_ARGB32);
#ifdef QT_DEBUG
    image.fill(0);
#endif

    QPen pen;

    if(p){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(  0, 100, 255, 77*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(  0, 100, 255, 50*(y-begy)/yrange));
        }
        pen.setColor(QColor(  0, 100, 255, 50));
    } else if(h){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 255, 77*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 255, 50*(y-begy)/yrange));
        }
        pen.setColor(QColor(255, 255, 255, 50));
    } else if(l){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 200, 44*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 200, 30*(y-begy)/yrange));
        }
        pen.setColor(QColor(255, 255, 200, 30));
    }

    painter->drawImage(QPoint(), image);

    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(pen);
    painter->drawLine(x1, y1e,
                      x2, y2e);
    painter->drawLine(x1, y1e-1,
                      x2, y2e-1);
    painter->restore();
}

void GlassStyle::Render(InPlaceNotifier *notifier, QPainter *painter){
    if(!notifier->GetNode()) return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    const QRectF bound = notifier->boundingRect();
    const QRectF rect = QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QImage image = notifier->GetNode()->GetImage();
    const QString title = notifier->GetNode()->GetTitle().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString url = notifier->GetNode()->GetUrl().toString().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString create = notifier->GetNode()->GetCreateDate().toString(Qt::SystemLocaleLongDate);
    const QString lastUpdate = notifier->GetNode()->GetLastUpdateDate().toString(Qt::SystemLocaleLongDate);
    const QString lastAccess = notifier->GetNode()->GetLastAccessDate().toString(Qt::SystemLocaleLongDate);

    if(image.isNull()){
        Node *tempnode = notifier->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    const QColor black(0,0,0,128);
    const QBrush brush(black);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(rect);

    if(m_InPlaceNotifierDrawBorder){
        painter->setPen(QColor(255,255,255,255));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(notifier->boundingRect());
    }

    const QRect image_rect =
        QRect(QPoint(m_ThumbnailPaddingX,
                     m_ThumbnailPaddingY),
              DEFAULT_THUMBNAIL_SIZE);

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        painter->drawImage(QRect(image_rect.topLeft()
                                 + QPoint(width_diff/2, height_diff/2),
                                 size),
                           image,
                           QRect(QPoint(), image.size()));
    } else if(notifier->GetNode()->IsDirectory()){
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 100, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 120, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    painter->setPen(QColor(255,255,255,255));
    painter->setBrush(QColor(255,255,255,100));
    painter->setFont(NOTIFIER_FONT);

    const int basex = m_ThumbnailPaddingX * 3 + image_rect.size().width();
    const int basey = 17;
    const int width = 495;
    const int height = 25;

    painter->drawText(QRect(basex, basey+25*0, width, height), Qt::AlignLeft, QObject::tr("Title : ") + title);
    painter->drawText(QRect(basex, basey+25*1, width, height), Qt::AlignLeft, QObject::tr("Url : ") + url);
    painter->drawText(QRect(basex, basey+25*2, width, height), Qt::AlignLeft, QObject::tr("CreatedDate : ") + create);
    painter->drawText(QRect(basex, basey+25*4, width, height), Qt::AlignLeft, QObject::tr("LastUpdatedDate : ") + lastUpdate);
    painter->drawText(QRect(basex, basey+25*3, width, height), Qt::AlignLeft, QObject::tr("LastAccessedDate : ") + lastAccess);

    painter->restore();
}

void GlassStyle::Render(UpDirectoryButton *button, QPainter *painter){
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor white(255, 255, 255, button->GetHovered() ? 200 : 100);
    QBrush brush(white);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(button->boundingRect(), 3, 3);

    painter->drawPixmap(QRect(QPoint(4,4), QSize(10, 10)), button->GetIcon(), QRect(QPoint(), button->GetIcon().size()));

    painter->restore();
}

void GlassStyle::Render(AccessibleWebElement *awe, QPainter *painter){
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0,0,0, awe->IsCurrentBlock() ? 100 : 50));
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(QColor(255,255,255, awe->IsCurrentBlock() ? 200 : 100));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(awe->CharChipRect(), Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(awe->GetBoundingPos() - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        QRect rect = QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT));

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0,0,0,100));
        painter->drawRect(rect);

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, ACCESSKEY_INFO_HEIGHT));

        painter->setFont(ACCESSKEY_INFO_FONT);
        painter->setPen(QColor(255,255,255,200));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(rect, str);

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        foreach(QString action, awe->GetGadgets()->GetAccessKeyKeyMap().values().toSet()){

            QStringList list;

            foreach(QKeySequence seq, awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0,0,0,100));
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            painter->setPen(QColor(255,255,255,200));
            painter->setBrush(Qt::NoBrush);

            painter->setFont(ACCESSKEY_CHAR_CHIP_S_FONT);
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(ACCESSKEY_INFO_FONT);
            painter->drawText(exprect.translated(QPoint(0,-1)), Qt::AlignCenter, exp);
        }
    }
    painter->restore();
}

void GlassStyle::OnSetNest(Thumbnail *thumb, int nest){
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    thumb->setOpacity(opacity);
}

void GlassStyle::OnSetNest(NodeTitle *title, int nest){
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    title->setOpacity(opacity);
}

void GlassStyle::OnSetPrimary(Thumbnail *thumb, bool){
    if(thumb->graphicsEffect()) thumb->setGraphicsEffect(0);
}

void GlassStyle::OnSetPrimary(NodeTitle *title, bool){
    if(title->graphicsEffect()) title->setGraphicsEffect(0);
}

void GlassStyle::OnSetHovered(Thumbnail *thumb, bool hovered){
    Q_UNUSED(thumb); Q_UNUSED(hovered);
}

void GlassStyle::OnSetHovered(NodeTitle *title, bool hovered){
    Q_UNUSED(title); Q_UNUSED(hovered);
}

void GlassStyle::OnSetHovered(UpDirectoryButton *button, bool hovered){
    if(button->graphicsEffect()) button->setGraphicsEffect(0);
    if(hovered){
        button->setCursor(Qt::PointingHandCursor);
    } else {
        button->setCursor(Qt::ArrowCursor);
    }
    button->update();
}

void GlassStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem){
    Q_UNUSED(elem);
    if(awe->graphicsEffect()) awe->setGraphicsEffect(0);
}

void GlassStyle::OnReshow(QGraphicsRectItem *gri){
    gri->setPen(QPen(QColor(255,255,255,255)));
    gri->setBrush(QBrush(QColor(255,255,255,50)));
}

QRectF GlassStyle::ThumbnailAreaRect(GraphicsTableView *gtv){
    return QRectF(DISPLAY_PADDING_X, 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF GlassStyle::NodeTitleAreaRect(GraphicsTableView *gtv){
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN * 2
                  + GADGETS_SCROLL_BAR_WIDTH,
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_Y : 0),
                  gtv->m_Size.width()
                  - DISPLAY_PADDING_X
                  - gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  - GADGETS_SCROLL_BAR_MARGIN * 2
                  - GADGETS_SCROLL_BAR_WIDTH -
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_X : 0),
                  gtv->m_Size.height() -
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_Y * 2 : 0));
}

QRectF GlassStyle::ScrollBarAreaRect(GraphicsTableView *gtv){
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN,
                  DISPLAY_PADDING_Y,
                  GADGETS_SCROLL_BAR_WIDTH,
                  gtv->m_Size.height() - DISPLAY_PADDING_Y * 2);
}

QGraphicsRectItem *GlassStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos){
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 QPen(QColor(255,255,255,255)),
                                 QBrush(QColor(255,255,255,50)));
}


QFont FlatStyle::m_ThumbnailTitleFont = QFont(DEFAULT_FONT, 10);

int FlatStyle::m_ThumbnailPaddingX = 15;
int FlatStyle::m_ThumbnailPaddingY = 15;
int FlatStyle::m_ThumbnailTitleHeight = 20;
int FlatStyle::m_ThumbnailWidthPercentage = 20;
int FlatStyle::m_ThumbnailDefaultColumnCount = 5;
int FlatStyle::m_ThumbnailAreaWidthPercentage =
    FlatStyle::m_ThumbnailWidthPercentage *
    FlatStyle::m_ThumbnailDefaultColumnCount;

bool FlatStyle::m_ThumbnailDrawBorder = false;

QSize FlatStyle::m_DefaultThumbnailWholeSize =
    QSize(DEFAULT_THUMBNAIL_SIZE.width()  + FlatStyle::m_ThumbnailPaddingX * 2,
          DEFAULT_THUMBNAIL_SIZE.height() + FlatStyle::m_ThumbnailPaddingY * 2);

QSize FlatStyle::m_MinimumThumbnailWholeSize =
    QSize(MINIMUM_THUMBNAIL_SIZE.width()  + FlatStyle::m_ThumbnailPaddingX * 2,
          MINIMUM_THUMBNAIL_SIZE.height() + FlatStyle::m_ThumbnailPaddingY * 2);

void FlatStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight){
    const float zoom = gtv->m_CurrentThumbnailZoomFactor; // alias

    int wholeWidth = gtv->m_Size.width() - DISPLAY_PADDING_X * 2;
    int areaWidth = wholeWidth;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = m_MinimumThumbnailWholeSize.width();
    int defWidth = m_DefaultThumbnailWholeSize.width() * zoom;

    if(col < 1) col = 1;

    if(defWidth < minWidth) defWidth = minWidth;

    if(thumbWidth > defWidth){
        col = areaWidth / defWidth;
    }

    if(thumbWidth < minWidth){
        col -= ((minWidth - thumbWidth) * col) / minWidth;
        if(col > 1) col -= 1;
        if(col == 0) col = 1;
    }

    thumbWidth = areaWidth / col;

    // compute height from width.
    const int marginWidth  = m_DefaultThumbnailWholeSize.width()  - DEFAULT_THUMBNAIL_SIZE.width();
    const int marginHeight = m_DefaultThumbnailWholeSize.height() - DEFAULT_THUMBNAIL_SIZE.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - DISPLAY_PADDING_Y) / thumbHeight;

    if(line < 1) line = 1;
}

void FlatStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter){
    QColor white(255, 255, 255, gtv->IsDisplayingNode() ? 170 : 100);
    QBrush brush(white);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
}

void FlatStyle::Render(Thumbnail *thumb, QPainter *painter){
    if(!thumb->GetNode() ||
       !thumb->boundingRect().translated(thumb->pos()).intersects(thumb->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    const QRectF bound = thumb->boundingRect();
    const QSize size = bound.size().toSize();
    const QRectF rect = QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QImage image = thumb->GetNode()->GetImage();
    const QString title = thumb->GetNode()->GetTitle();
    const QUrl url = thumb->GetNode()->GetUrl();

    if(image.isNull()){
        Node *tempnode = thumb->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    if(m_ThumbnailDrawBorder){
        painter->setPen(QColor(255,255,255,255));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    QRect image_rect =
        QRect(bound.topLeft().toPoint() +
              QPoint(m_ThumbnailPaddingX,
                     m_ThumbnailPaddingY),
              QSize(size.width()
                    - m_ThumbnailPaddingX * 2,
                    size.height()
                    - m_ThumbnailPaddingY * 2)
              - QSize(1,1));

    QRect title_rect =
        QRect(bound.topLeft().toPoint() +
              QPoint(m_ThumbnailPaddingX,
                     size.height()
                     - m_ThumbnailPaddingY
                     - m_ThumbnailTitleHeight),
              QSize(size.width()
                    - m_ThumbnailPaddingX * 2,
                    m_ThumbnailTitleHeight)
              - QSize(2,1));

    // hovered : enlarge
    // primary : effect
    // selected : frame
    // loaded : none

    image_rect = QRect(image_rect.topLeft() - QPoint(3, 3),
                       image_rect.size() + QSize(6, 6));
    title_rect = QRect(title_rect.topLeft() + QPoint(-3, 2),
                       title_rect.size() + QSize(7, 0));

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        painter->drawImage(QRect(image_rect.topLeft()
                                 + QPoint(width_diff/2, height_diff/2),
                                 size),
                           image,
                           QRect(QPoint(), image.size()));
    } else if(thumb->GetNode()->IsDirectory()){
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 100, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 120, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(0,0,0,128)));
    painter->drawRect(title_rect);

    painter->setFont(m_ThumbnailTitleFont);
    painter->setPen(QPen(QColor(255,255,255,255)));
    painter->setBrush(Qt::NoBrush);

    title_rect = QRect(title_rect.topLeft() + QPoint(2,0),
                       title_rect.size() - QSize(4,0));

    if(!title.isEmpty()){
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter,
                          thumb->GetNode()->IsDirectory() ?
                          QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first() : title);
    } else {
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter,
                          !url.isEmpty() ? url.toString() :
                          thumb->GetNode()->IsDirectory() ? QStringLiteral("Directory") : QStringLiteral("No Title"));
    }

    if(thumb->isSelected()){
        painter->setPen(QColor(100,100,255,255));
        painter->setBrush(QColor(100,100,255,50));
        painter->drawRect(QRect(image_rect.topLeft(),
                                image_rect.size()-QSize(1,1)));
    }
    painter->restore();
}

void FlatStyle::Render(NodeTitle *title, QPainter *painter){
    Q_UNUSED(title); Q_UNUSED(painter);
}

void FlatStyle::Render(SpotLight *title, QPainter *painter){
    Q_UNUSED(title); Q_UNUSED(painter);
}

void FlatStyle::Render(InPlaceNotifier *notifier, QPainter *painter){
    Q_UNUSED(notifier); Q_UNUSED(painter);
}

void FlatStyle::Render(UpDirectoryButton *button, QPainter *painter){
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor white(255, 255, 255, 255);
    QBrush brush(white);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(button->boundingRect(), 3, 3);

    painter->drawPixmap(QRect(QPoint(4,4), QSize(10, 10)), button->GetIcon(), QRect(QPoint(), button->GetIcon().size()));

    painter->restore();
}

void FlatStyle::Render(AccessibleWebElement *awe, QPainter *painter){
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255,255,255,255));
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(QColor(0,0,0, awe->IsCurrentBlock() ? 255 : 127));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(awe->CharChipRect(), Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(awe->GetBoundingPos() - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        QRect rect = QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT));

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255,255,255,255));
        painter->drawRect(rect);

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, ACCESSKEY_INFO_HEIGHT));

        painter->setFont(ACCESSKEY_INFO_FONT);
        painter->setPen(QColor(0,0,0,255));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(rect, str);

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        foreach(QString action, awe->GetGadgets()->GetAccessKeyKeyMap().values().toSet()){

            QStringList list;

            foreach(QKeySequence seq, awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255,255,255,255));
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            painter->setPen(QColor(0,0,0,255));
            painter->setBrush(Qt::NoBrush);

            painter->setFont(ACCESSKEY_CHAR_CHIP_S_FONT);
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(ACCESSKEY_INFO_FONT);
            painter->drawText(exprect.translated(QPoint(0,-1)), Qt::AlignCenter, exp);
        }
    }
    painter->restore();
}

void FlatStyle::OnSetNest(Thumbnail *thumb, int nest){
    Q_UNUSED(thumb); Q_UNUSED(nest);
}

void FlatStyle::OnSetNest(NodeTitle *title, int nest){
    Q_UNUSED(title); Q_UNUSED(nest);
}

void FlatStyle::OnSetPrimary(Thumbnail *thumb, bool primary){
    if(!thumb->graphicsEffect()){
        QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
        thumb->setGraphicsEffect(effect);
        effect->setBlurRadius(10);
        effect->setOffset(QPointF());
    }
    if(primary){
        static_cast<QGraphicsDropShadowEffect*>(thumb->graphicsEffect())->setColor(QColor(0,0,100,255));
    } else {
        static_cast<QGraphicsDropShadowEffect*>(thumb->graphicsEffect())->setColor(QColor(0,0,0,255));
    }
}

void FlatStyle::OnSetPrimary(NodeTitle *title, bool primary){
    Q_UNUSED(title); Q_UNUSED(primary);
}

void FlatStyle::OnSetHovered(Thumbnail *thumb, bool hovered){
    Q_UNUSED(thumb); Q_UNUSED(hovered);
}

void FlatStyle::OnSetHovered(NodeTitle *title, bool hovered){
    Q_UNUSED(title); Q_UNUSED(hovered);
}

void FlatStyle::OnSetHovered(UpDirectoryButton *button, bool hovered){
    if(!button->graphicsEffect()){
        QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
        button->setGraphicsEffect(effect);
        effect->setBlurRadius(10);
        effect->setOffset(QPointF());
    }
    if(hovered){
        static_cast<QGraphicsDropShadowEffect*>(button->graphicsEffect())->setColor(QColor(0,0,0,255));
        button->setCursor(Qt::PointingHandCursor);
    } else {
        static_cast<QGraphicsDropShadowEffect*>(button->graphicsEffect())->setColor(QColor(0,0,0,127));
        button->setCursor(Qt::ArrowCursor);
    }
    button->update();
}

void FlatStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem){
    Q_UNUSED(elem);
    if(awe->graphicsEffect()) return;
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
    effect->setColor(QColor(0,0,0,255));
    effect->setBlurRadius(10);
    effect->setOffset(QPointF());
    awe->setGraphicsEffect(effect);
}

void FlatStyle::OnReshow(QGraphicsRectItem *gri){
    gri->setPen(QPen(QColor(100,100,255,255)));
    gri->setBrush(QBrush(QColor(100,100,255,50)));
}

QRectF FlatStyle::ThumbnailAreaRect(GraphicsTableView *gtv){
    return QRectF(DISPLAY_PADDING_X, 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF FlatStyle::NodeTitleAreaRect(GraphicsTableView *gtv){
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN * 2
                  + GADGETS_SCROLL_BAR_WIDTH,
                  DISPLAY_PADDING_Y,
                  0,0);
}

QRectF FlatStyle::ScrollBarAreaRect(GraphicsTableView *gtv){
    return QRectF(gtv->m_Size.width() - DISPLAY_PADDING_X - 1
                  + GADGETS_SCROLL_BAR_MARGIN,
                  DISPLAY_PADDING_Y,
                  GADGETS_SCROLL_BAR_WIDTH,
                  gtv->m_Size.height() - DISPLAY_PADDING_Y * 2);
}

QGraphicsRectItem *FlatStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos){
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 QPen(QColor(100,100,255,255)),
                                 QBrush(QColor(100,100,255,50)));
}
