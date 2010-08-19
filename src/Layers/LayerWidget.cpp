#include "LayerWidget.h"
#include "LayerDock.h"

#include "MainWindow.h"
#include "Document.h"
#include "Layer.h"
#include "Preferences/MerkaartorPreferences.h"

#include "IMapAdapter.h"

#include <QApplication>
#include <QMouseEvent>
#include <QStylePainter>

#define LINEHEIGHT 25

LayerWidget::LayerWidget(Layer* aLayer, QWidget* aParent)
: QPushButton(aParent), theLayer(aLayer), ctxMenu(0), closeAction(0), actZoom(0), associatedMenu(0)
{
    ui.setupUi(this);

    setCheckable(true);
//    setFlat(true);
    setFocusPolicy(Qt::NoFocus);
    setContextMenuPolicy(Qt::NoContextMenu);

    ui.cbVisible->blockSignals(true);
    ui.cbVisible->setChecked(theLayer->isVisible());
    ui.cbVisible->blockSignals(false);

    ui.edName->setText(theLayer->name());
    ui.edName->setReadOnly(true);
    ui.edName->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui.edName->setFrame(false);
    ui.edName->setCursorPosition(0);
    ui.edName->setStyleSheet(" background: transparent; ");
}

LayerWidget::~LayerWidget()
{
    delete associatedMenu;
}

void LayerWidget::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);
    p.drawControl(QStyle::CE_PushButton, option);

    if (!theLayer->isUploadable()) {
        p.fillRect(rect().adjusted(20,0,0,-1),QBrush(Qt::red, Qt::BDiagPattern));
    }

}

void LayerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        dragStartPosition = event->pos();

    event->ignore();
}

void LayerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
        < QApplication::startDragDistance())
        return;

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-layer", theLayer->id().toLatin1());
    drag->setMimeData(mimeData);

    QPixmap px(size());
    render(&px);
    drag->setPixmap(px);

    /*Qt::DropAction dropAction =*/ drag->exec(Qt::MoveAction);
}

void LayerWidget::mouseReleaseEvent(QMouseEvent* anEvent)
{
    anEvent->ignore();
}

void LayerWidget::on_cbVisible_stateChanged ( int state )
{
    setLayerVisible((state > Qt::Unchecked));
}

void LayerWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    ui.edName->setReadOnly(false);
    ui.edName->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    ui.edName->setFrame(true);
    ui.edName->setStyleSheet("");
    ui.edName->setFocus();
}

void LayerWidget::on_edName_editingFinished()
{
    ui.edName->setReadOnly(true);
    ui.edName->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui.edName->setFrame(false);
    ui.edName->setStyleSheet(" background: transparent; ");

    if (!ui.edName->text().isEmpty())
        theLayer->setName(ui.edName->text());
    else
        ui.edName->setText(theLayer->name());
}
void LayerWidget::checkStateSet()
{
    QAbstractButton::checkStateSet();
    //emit (layerSelected(this));
}

void LayerWidget::nextCheckState()
{
    QAbstractButton::nextCheckState();
    theLayer->setSelected(isChecked());
    //emit (layerSelected(this));
}

void LayerWidget::setName(const QString& s)
{
    ui.edName->setText(s);
}

Layer* LayerWidget::getMapLayer()
{
    return theLayer;
}

void LayerWidget::showContextMenu(QContextMenuEvent* anEvent)
{
    //initActions();

    if (ctxMenu) {
        if (actZoom) {
            actZoom->setVisible(true);
            ImageMapLayer* il = dynamic_cast<ImageMapLayer *>(theLayer.data());
            if (il) {
                if (!il->getMapAdapter() || il->getMapAdapter()->getBoundingbox().isNull())
                    actZoom->setVisible(false);
            } else
                actZoom->setEnabled(theLayer->size());
        }
        if (closeAction)
            closeAction->setEnabled(theLayer->canDelete());
        ctxMenu->exec(anEvent->globalPos());
    }
}

#define NUMOP 3
void LayerWidget::initActions()
{
    SAFE_DELETE(ctxMenu);
    SAFE_DELETE(associatedMenu);

    ctxMenu = new QMenu(this);
    associatedMenu = new QMenu(theLayer->name());
    //connect(associatedMenu, SIGNAL(aboutToShow()), this, SLOT(associatedAboutToShow()));

    actVisible = new QAction(tr("Visible"), ctxMenu);
    actVisible->setCheckable(true);
    actVisible->setChecked(theLayer->isVisible());
    associatedMenu->addAction(actVisible);
    connect(actVisible, SIGNAL(triggered(bool)), this, SLOT(visibleLayer(bool)));

    actReadonly = new QAction(tr("Readonly"), ctxMenu);
    actReadonly->setCheckable(true);
    actReadonly->setChecked(theLayer->isReadonly());
    associatedMenu->addAction(actReadonly);
    ctxMenu->addAction(actReadonly);
    connect(actReadonly, SIGNAL(triggered(bool)), this, SLOT(readonlyLayer(bool)));

    static const char *opStr[NUMOP] = {
    QT_TR_NOOP("Low"), QT_TR_NOOP("High"), QT_TR_NOOP("Opaque")};

    QActionGroup* actgrp = new QActionGroup(this);
    QMenu* alphaMenu = new QMenu(tr("Opacity"), this);
    for (int i=0; i<NUMOP; i++) {
        QAction* act = new QAction(tr(opStr[i]), alphaMenu);
        actgrp->addAction(act);
        qreal a = M_PREFS->getAlpha(opStr[i]);
        act->setData(a);
        act->setCheckable(true);
        if (int(theLayer->getAlpha()*100) == int(a*100))
            act->setChecked(true);
        alphaMenu->addAction(act);
    }
    ctxMenu->addMenu(alphaMenu);
    connect(alphaMenu, SIGNAL(triggered(QAction*)), this, SLOT(setOpacity(QAction*)));
    associatedMenu->addMenu(alphaMenu);

    actZoom = new QAction(tr("Zoom"), ctxMenu);
    ctxMenu->addAction(actZoom);
    associatedMenu->addAction(actZoom);
    connect(actZoom, SIGNAL(triggered(bool)), this, SLOT(zoomLayer(bool)));
}

void LayerWidget::setOpacity(QAction *act)
{
    theLayer->setAlpha(act->data().toDouble());
    emit (layerChanged(this, false));
}

void LayerWidget::close()
{
    emit(layerClosed(theLayer));
}

void LayerWidget::clear()
{
    emit(layerCleared(theLayer));
}

void LayerWidget::zoomLayer(bool)
{
    emit (layerZoom(theLayer));
}

void LayerWidget::visibleLayer(bool)
{
    setLayerVisible(actVisible->isChecked());
}

void LayerWidget::readonlyLayer(bool)
{
    setLayerReadonly(actReadonly->isChecked());
}

QMenu* LayerWidget::getAssociatedMenu()
{
    return associatedMenu;
}

void LayerWidget::setLayerVisible(bool b, bool updateLayer)
{
    if (updateLayer)
        theLayer->setVisible(b);
    actVisible->setChecked(b);
    ui.cbVisible->setChecked(b);
    update();
    emit(layerChanged(this, false));
}

void LayerWidget::setLayerReadonly(bool b)
{
    theLayer->setReadonly(b);
    actReadonly->setChecked(b);
    update();
    emit(layerChanged(this, false));
}

void LayerWidget::associatedAboutToShow()
{
    initActions();
}


// DrawingLayerWidget

DrawingLayerWidget::DrawingLayerWidget(DrawingLayer* aLayer, QWidget* aParent)
    : LayerWidget(aLayer, aParent)
{
    initActions();
}

void DrawingLayerWidget::initActions()
{
    LayerWidget::initActions();

    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    closeAction = new QAction(tr("Close"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));
    ctxMenu->addAction(closeAction);
    associatedMenu->addAction(closeAction);
    closeAction->setEnabled(theLayer->canDelete());
}

// ImageLayerWidget

ImageLayerWidget::ImageLayerWidget(ImageMapLayer* aLayer, QWidget* aParent)
: LayerWidget(aLayer, aParent), wmsMenu(0) //, actgrWms(0)
{
    actNone = new QAction(tr("None"), this);
    //actNone->setCheckable(true);
    actNone->setChecked((M_PREFS->getBackgroundPlugin() == NONE_ADAPTER_UUID));
    actNone->setData(QVariant::fromValue(NONE_ADAPTER_UUID));

    if (M_PREFS->getUseShapefileForBackground()) {
        actShape = new QAction(tr("Shape adapter"), this);
        //actShape->setCheckable(true);
        actShape->setChecked((M_PREFS->getBackgroundPlugin() == SHAPE_ADAPTER_UUID));
        actShape->setData(QVariant::fromValue(SHAPE_ADAPTER_UUID));
    }

    initActions();
}

ImageLayerWidget::~ImageLayerWidget()
{
}

void ImageLayerWidget::setWms(QAction* act)
{
    WmsServerList* L = M_PREFS->getWmsServers();
    WmsServer S = L->value(act->data().toString());
    M_PREFS->setSelectedServer(S.WmsName);

    ((ImageMapLayer *)theLayer.data())->setMapAdapter(WMS_ADAPTER_UUID, S.WmsName);
    theLayer->setVisible(true);

    this->update(rect());
    emit (layerChanged(this, true));
}

void ImageLayerWidget::setTms(QAction* act)
{
    TmsServerList* L = M_PREFS->getTmsServers();
    TmsServer S = L->value(act->data().toString());
    M_PREFS->setSelectedServer(S.TmsName);

    ((ImageMapLayer *)theLayer.data())->setMapAdapter(TMS_ADAPTER_UUID, S.TmsName);
    theLayer->setVisible(true);

    this->update(rect());
    emit (layerChanged(this, true));
}

void ImageLayerWidget::setOther(QAction* act)
{
    QUuid u(act->data().toString());

    ((ImageMapLayer *)theLayer.data())->setMapAdapter(u);
    theLayer->setVisible(true);

    this->update(rect());
    emit (layerChanged(this, true));
}

void ImageLayerWidget::setBackground(QAction* act)
{
    QUuid aUuid = act->data().value<QUuid>();
    if (aUuid.isNull())
        return;

    ((ImageMapLayer *)theLayer.data())->setMapAdapter(aUuid);

    this->update(rect());
    emit (layerChanged(this, true));
}

void ImageLayerWidget::setProjection()
{
    IMapAdapter* ma = ((ImageMapLayer*)theLayer.data())->getMapAdapter();
    if (ma) {
        emit (layerProjection(ma->projection()));
    }
}

void ImageLayerWidget::initActions()
{
    //if (actgrWms)
    //	delete actgrWms;
    //actgrWms = new QActionGroup(this);

    LayerWidget::initActions();
    ImageMapLayer* il = ((ImageMapLayer *)theLayer.data());
    if (il && il->boundingBox().isNull())
        actZoom->setVisible(false);

    actReadonly->setVisible(false);

    actProjection = new QAction(tr("Set view projection to layer's"), this);
    connect(actProjection, SIGNAL(triggered()), this, SLOT(setProjection()));
    ctxMenu->addAction(actProjection);
    associatedMenu->addAction(actProjection);

    closeAction = new QAction(tr("Close"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));
    ctxMenu->addAction(closeAction);
    associatedMenu->addAction(closeAction);

    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    wmsMenu = new QMenu(tr("WMS adapter"), this);
    WmsServerList* WmsServers = M_PREFS->getWmsServers();
    WmsServerListIterator wi(*WmsServers);
    while (wi.hasNext()) {
        wi.next();
        WmsServer S = wi.value();
        if (!S.deleted) {
            QAction* act = new QAction(S.WmsName, wmsMenu);
            act->setData(S.WmsName);
            wmsMenu->addAction(act);
            if (M_PREFS->getBackgroundPlugin() == WMS_ADAPTER_UUID)
                if (S.WmsName == M_PREFS->getSelectedServer())
                    act->setChecked(true);
        }
    }

    tmsMenu = new QMenu(tr("TMS adapter"), this);
    TmsServerList* TmsServers = M_PREFS->getTmsServers();
    TmsServerListIterator ti(*TmsServers);
    while (ti.hasNext()) {
        ti.next();
        TmsServer S = ti.value();
        if (!S.deleted) {
            QAction* act = new QAction(S.TmsName, tmsMenu);
            act->setData(S.TmsName);
            tmsMenu->addAction(act);
            if (M_PREFS->getBackgroundPlugin() == TMS_ADAPTER_UUID)
                if (S.TmsName == M_PREFS->getSelectedServer())
                    act->setChecked(true);
        }
    }

    actNone->setChecked((M_PREFS->getBackgroundPlugin() == NONE_ADAPTER_UUID));
    if (M_PREFS->getUseShapefileForBackground())
        actShape->setChecked((M_PREFS->getBackgroundPlugin() == SHAPE_ADAPTER_UUID));

    ctxMenu->addAction(actNone);
    associatedMenu->addAction(actNone);

    ctxMenu->addMenu(wmsMenu);
    associatedMenu->addMenu(wmsMenu);
    connect(wmsMenu, SIGNAL(triggered(QAction*)), this, SLOT(setWms(QAction*)));

    ctxMenu->addMenu(tmsMenu);
    associatedMenu->addMenu(tmsMenu);
    connect(tmsMenu, SIGNAL(triggered(QAction*)), this, SLOT(setTms(QAction*)));

    if (M_PREFS->getUseShapefileForBackground()) {
        ctxMenu->addAction(actShape);
        associatedMenu->addAction(actShape);
    }

    QMapIterator <QUuid, IMapAdapter *> it(M_PREFS->getBackgroundPlugins());
    while (it.hasNext()) {
        it.next();

        QAction* actBackPlug = new QAction(it.value()->getName(), this);
        actBackPlug->setChecked((M_PREFS->getBackgroundPlugin() == it.key()));
        actBackPlug->setData(QVariant::fromValue(it.key()));

        if (it.value()->getMenu()) {
            actBackPlug->setMenu(it.value()->getMenu());
            disconnect(it.value()->getMenu(), 0, 0, 0);
            connect(it.value()->getMenu(), SIGNAL(triggered(QAction*)), SLOT(setOther(QAction*)));
        }

        ctxMenu->addAction(actBackPlug);
        associatedMenu->addAction(actBackPlug);
    }
    connect(ctxMenu, SIGNAL(triggered(QAction*)), this, SLOT(setBackground(QAction*)));
}

// TrackLayerWidget

TrackLayerWidget::TrackLayerWidget(TrackLayer* aLayer, QWidget* aParent)
    : LayerWidget(aLayer, aParent)
{
    initActions();
}

void TrackLayerWidget::initActions()
{
    LayerWidget::initActions();
    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    QAction* actExtract = new QAction(tr("Extract Drawing layer"), ctxMenu);
    ctxMenu->addAction(actExtract);
    associatedMenu->addAction(actExtract);
    connect(actExtract, SIGNAL(triggered(bool)), this, SLOT(extractLayer(bool)));

    actZoom = new QAction(tr("Zoom"), ctxMenu);
    ctxMenu->addAction(actZoom);
    associatedMenu->addAction(actZoom);
    connect(actZoom, SIGNAL(triggered(bool)), this, SLOT(zoomLayer(bool)));

    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    closeAction = new QAction(tr("Close"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));
    ctxMenu->addAction(closeAction);
    associatedMenu->addAction(closeAction);
    closeAction->setEnabled(theLayer->canDelete());
}

TrackLayerWidget::~TrackLayerWidget()
{
}

void TrackLayerWidget::extractLayer(bool)
{
    ((TrackLayer*)theLayer.data())->extractLayer();
    emit (layerChanged(this, false));
}

// DirtyLayerWidget

DirtyLayerWidget::DirtyLayerWidget(DirtyLayer* aLayer, QWidget* aParent)
    : LayerWidget(aLayer, aParent)
{
    initActions();
}

void DirtyLayerWidget::initActions()
{
    LayerWidget::initActions();
    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    actZoom = new QAction(tr("Zoom"), ctxMenu);
    ctxMenu->addAction(actZoom);
    associatedMenu->addAction(actZoom);
    connect(actZoom, SIGNAL(triggered(bool)), this, SLOT(zoomLayer(bool)));
}

// UploadLayerWidget

UploadedLayerWidget::UploadedLayerWidget(UploadedLayer* aLayer, QWidget* aParent)
    : LayerWidget(aLayer, aParent)
{
    initActions();
}

void UploadedLayerWidget::initActions()
{
    LayerWidget::initActions();
    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    actZoom = new QAction(tr("Zoom"), ctxMenu);
    ctxMenu->addAction(actZoom);
    associatedMenu->addAction(actZoom);
    connect(actZoom, SIGNAL(triggered(bool)), this, SLOT(zoomLayer(bool)));

    closeAction = new QAction(tr("Clear"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(clear()));
    ctxMenu->addAction(closeAction);
    associatedMenu->addAction(closeAction);
    closeAction->setEnabled(theLayer->canDelete());
}

// OsbLayerWidget

OsbLayerWidget::OsbLayerWidget(OsbLayer* aLayer, QWidget* aParent)
    : LayerWidget(aLayer, aParent)
{
    initActions();
}

void OsbLayerWidget::initActions()
{
    LayerWidget::initActions();

    ctxMenu->addSeparator();
    associatedMenu->addSeparator();

    closeAction = new QAction(tr("Close"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));
    ctxMenu->addAction(closeAction);
    associatedMenu->addAction(closeAction);
    closeAction->setEnabled(theLayer->canDelete());
}

