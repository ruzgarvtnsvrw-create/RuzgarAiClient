// =============================================================================
//  RÜZGAR AI - Qt 6 Widgets masaüstü sohbet uygulaması (TEK DOSYA: main.cpp)
//  Modüller: Core, Gui, Widgets, Network, Sql   |   C++17, MSVC 2022, Qt 6.2+
//  Dosyayı UTF-8 kaydedin ve derleyiciye /utf-8 verin (Türkçe karakterler için).
// =============================================================================
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <QtWidgets>
#include <QtNetwork>
#include <QtSql>
#include <QtMath>
#include <QShortcut>
#include <functional>
#include <string>
#include <cmath>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#pragma comment(lib, "advapi32.lib")
#endif

static const double kPi = 3.14159265358979323846;

// ============================================================================
// 1) Ayarlar, tema, ikonlar
// ============================================================================
namespace Cfg {
inline QSettings &s() { static QSettings q(QStringLiteral("RuzgarAI"), QStringLiteral("RuzgarAI")); return q; }
inline QVariant get(const QString &k, const QVariant &d = QVariant()) { return s().value(k, d); }
inline void set(const QString &k, const QVariant &v) { s().setValue(k, v); }
inline bool anim() { return get("anim", true).toBool(); }
}

struct Theme {
    bool dark = false;
    QColor bg, sidebar, composer, text, text2, border, hover, sel, accent, accentHi, bubble, codeBg, codeHead, danger, ok;
};
static Theme g_t;
static QString g_chk, g_chev;
static QColor C(const char *h) { return QColor(QString::fromLatin1(h)); }
static Theme makeTheme(bool d) {
    Theme t; t.dark = d;
    if (!d) {
        t.bg = C("#FFFFFF"); t.sidebar = C("#F7F7F7"); t.composer = C("#FFFFFF"); t.text = C("#202020"); t.text2 = C("#777777");
        t.border = C("#E5E5E5"); t.hover = C("#EEEEEE"); t.sel = C("#E7E7E7"); t.accent = C("#2D6CDF"); t.accentHi = C("#2459B8");
        t.bubble = C("#F4F4F4"); t.codeBg = C("#FAFAFA"); t.codeHead = C("#EFEFEF"); t.danger = C("#D93025"); t.ok = C("#1E8E3E");
    } else {
        t.bg = C("#212121"); t.sidebar = C("#171717"); t.composer = C("#303030"); t.text = C("#ECECEC"); t.text2 = C("#A1A1A1");
        t.border = C("#3C3C3C"); t.hover = C("#2B2B2B"); t.sel = C("#303030"); t.accent = C("#4C8DF6"); t.accentHi = C("#3B79DE");
        t.bubble = C("#2F2F2F"); t.codeBg = C("#171717"); t.codeHead = C("#2A2A2A"); t.danger = C("#F28B82"); t.ok = C("#81C995");
    }
    return t;
}
static QColor blendC(const QColor &a, const QColor &b, qreal t) {
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t, a.blueF() + (b.blueF() - a.blueF()) * t);
}

class ThemeBus : public QObject {
    Q_OBJECT
public:
    explicit ThemeBus(QObject *p) : QObject(p) {}
    static ThemeBus &I() { static ThemeBus *b = new ThemeBus(qApp); return *b; }
signals:
    void changed();
    void error(const QString &msg);
};

// ---------- Lucide benzeri vektör ikonlar (24x24 koordinat sistemi) ----------
static void paintIcon(QPainter &p, const QString &n, const QColor &c) {
    p.setBrush(Qt::NoBrush);
    auto L = [&](double a, double b, double x, double y) { p.drawLine(QPointF(a, b), QPointF(x, y)); };
    auto PL = [&](std::initializer_list<QPointF> pts) { p.drawPolyline(QPolygonF(QList<QPointF>(pts))); };
    auto PG = [&](std::initializer_list<QPointF> pts) { p.drawPolygon(QPolygonF(QList<QPointF>(pts))); };
    auto RR = [&](double x, double y, double w, double h, double r) { p.drawRoundedRect(QRectF(x, y, w, h), r, r); };
    auto CI = [&](double x, double y, double r) { p.drawEllipse(QPointF(x, y), r, r); };
    auto DOT = [&](double x, double y) { p.save(); p.setPen(Qt::NoPen); p.setBrush(c); p.drawEllipse(QPointF(x, y), 1.5, 1.5); p.restore(); };
    if (n == "plus") { L(12, 5, 12, 19); L(5, 12, 19, 12); }
    else if (n == "search") { CI(11, 11, 7); L(21, 21, 16.2, 16.2); }
    else if (n == "sidebar") { RR(3, 3, 18, 18, 3); L(9, 3, 9, 21); }
    else if (n == "newchat") {
        QPainterPath pp; pp.moveTo(12, 3.5); pp.lineTo(6.5, 3.5); pp.quadTo(3.5, 3.5, 3.5, 6.5); pp.lineTo(3.5, 17.5); pp.quadTo(3.5, 20.5, 6.5, 20.5);
        pp.lineTo(17.5, 20.5); pp.quadTo(20.5, 20.5, 20.5, 17.5); pp.lineTo(20.5, 12); p.drawPath(pp);
        PG({{9, 12.5}, {18, 3.5}, {20.5, 6}, {11.5, 15}, {8.5, 15.5}});
    }
    else if (n == "image") { RR(3, 3, 18, 18, 3); CI(9, 9, 1.6); PL({{21, 15}, {16, 10}, {5, 21}}); }
    else if (n == "clock") { CI(12, 12, 9); PL({{12, 7}, {12, 12}, {15.5, 14}}); }
    else if (n == "library") { p.drawRect(QRectF(3.5, 4, 3.5, 16)); p.drawRect(QRectF(9.5, 4, 3.5, 16)); L(16, 5.5, 20, 19); }
    else if (n == "grid") { CI(7, 7, 3); CI(17, 7, 3); CI(7, 17, 3); CI(17, 17, 3); }
    else if (n == "arrowup") { L(12, 19, 12, 5); PL({{5, 12}, {12, 5}, {19, 12}}); }
    else if (n == "arrowdown") { L(12, 5, 12, 19); PL({{5, 12}, {12, 19}, {19, 12}}); }
    else if (n == "stop") { p.save(); p.setBrush(c); RR(7, 7, 10, 10, 2); p.restore(); }
    else if (n == "mic") { RR(9, 2, 6, 12, 3); p.drawArc(QRectF(5, 4, 14, 14), 180 * 16, 180 * 16); L(12, 18, 12, 22); }
    else if (n == "copy") { RR(9, 9, 11.5, 11.5, 2); PL({{5, 15}, {4, 15}, {3, 14}, {3, 4}, {4, 3}, {14, 3}, {15, 4}, {15, 5}}); }
    else if (n == "check") { PL({{4, 12.5}, {9.5, 18}, {20, 6.5}}); }
    else if (n == "thumbup" || n == "thumbdown") {
        p.save(); if (n == "thumbdown") { p.translate(0, 24); p.scale(1, -1); }
        RR(3, 10.5, 4, 10.5, 1);
        PL({{7, 11}, {11, 3.5}, {12.5, 3.5}, {13.5, 5}, {13.5, 9}, {19.5, 9}, {21, 10.5}, {19.7, 18.5}, {18.2, 20.5}, {7, 20.5}});
        p.restore();
    }
    else if (n == "refresh") { p.drawArc(QRectF(4, 4, 16, 16), 90 * 16, -270 * 16); PL({{1.5, 11.5}, {4, 9}, {6.5, 11.5}}); }
    else if (n == "more") { DOT(5, 12); DOT(12, 12); DOT(19, 12); }
    else if (n == "chevdown") { PL({{6, 9}, {12, 15}, {18, 9}}); }
    else if (n == "chevup") { PL({{6, 15}, {12, 9}, {18, 15}}); }
    else if (n == "share") { L(12, 3, 12, 15); PL({{8, 7}, {12, 3}, {16, 7}}); PL({{5, 12}, {5, 20}, {19, 20}, {19, 12}}); }
    else if (n == "settings") { L(4, 6, 20, 6); CI(9, 6, 2.2); L(4, 12, 20, 12); CI(15, 12, 2.2); L(4, 18, 20, 18); CI(8, 18, 2.2); }
    else if (n == "user") { CI(12, 8, 4); p.drawArc(QRectF(4, 14, 16, 14), 0, 180 * 16); L(4, 21, 20, 21); }
    else if (n == "cpu") {
        RR(6, 6, 12, 12, 2); RR(9.5, 9.5, 5, 5, 1);
        L(9, 2, 9, 6); L(15, 2, 15, 6); L(9, 18, 9, 22); L(15, 18, 15, 22); L(2, 9, 6, 9); L(2, 15, 6, 15); L(18, 9, 22, 9); L(18, 15, 22, 15);
    }
    else if (n == "key") { CI(8, 15, 4); L(11, 12, 21, 3); L(18, 6, 21, 9); }
    else if (n == "sun") { CI(12, 12, 4); p.save(); p.translate(12, 12); for (int i = 0; i < 8; ++i) { p.drawLine(QPointF(0, -7), QPointF(0, -9.5)); p.rotate(45); } p.restore(); }
    else if (n == "info") { CI(12, 12, 9); L(12, 16, 12, 12); DOT(12, 8.3); }
    else if (n == "trash") { L(3, 6, 21, 6); PL({{5, 6}, {6, 20}, {18, 20}, {19, 6}}); PL({{9, 6}, {9, 3}, {15, 3}, {15, 6}}); L(10, 10, 10, 16); L(14, 10, 14, 16); }
    else if (n == "pin") { L(12, 16, 12, 22); PG({{9, 3}, {15, 3}, {14.5, 10}, {18, 14}, {6, 14}, {9.5, 10}}); }
    else if (n == "x") { L(6, 6, 18, 18); L(18, 6, 6, 18); }
    else if (n == "eye" || n == "eyeoff") {
        QPainterPath pp; pp.moveTo(2, 12); pp.quadTo(12, 0, 22, 12); pp.quadTo(12, 24, 2, 12); p.drawPath(pp); CI(12, 12, 3);
        if (n == "eyeoff") L(3, 3, 21, 21);
    }
    else if (n == "brain") { CI(12, 12, 9); L(12, 3, 12, 21); L(12, 9, 16, 9); L(12, 15, 8, 15); }
    else if (n == "folder") { RR(3, 6, 18, 14, 2.5); PL({{3, 6}, {3, 5}, {9, 5}, {11, 7}}); }
    else if (n == "chat") { RR(3, 4, 18, 13, 3); PL({{8, 17}, {8, 21}, {12, 17}}); }
    else if (n == "server") { RR(3, 4, 18, 7, 2); RR(3, 13, 18, 7, 2); L(7, 7.5, 7.01, 7.5); L(7, 16.5, 7.01, 16.5); }
    else if (n == "logout") { PL({{9, 3}, {5, 3}, {3, 5}, {3, 19}, {5, 21}, {9, 21}}); L(9, 12, 21, 12); PL({{16, 7}, {21, 12}, {16, 17}}); }
    else if (n == "wind") {
        QPainterPath a; a.moveTo(2, 8); a.lineTo(11, 8); a.cubicTo(14, 8, 14, 3.5, 11, 3.5); p.drawPath(a);
        QPainterPath b; b.moveTo(2, 12); b.lineTo(19, 12); b.cubicTo(22.5, 12, 22.5, 7, 19, 7); p.drawPath(b);
        QPainterPath d; d.moveTo(2, 16); d.lineTo(12, 16); d.cubicTo(15, 16, 15, 20.5, 12, 20.5); p.drawPath(d);
    }
}
static QPixmap iconPix(const QString &n, int sz, const QColor &c) {
    static QHash<QString, QPixmap> cache;
    const QString key = n + QLatin1Char('|') + QString::number(sz) + QLatin1Char('|') + c.name(QColor::HexArgb);
    auto it = cache.constFind(key);
    if (it != cache.constEnd()) return *it;
    const int dpr = 2;
    QPixmap pm(sz * dpr, sz * dpr); pm.fill(Qt::transparent); pm.setDevicePixelRatio(dpr);
    QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing); p.scale(sz / 24.0, sz / 24.0);
    p.setPen(QPen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    paintIcon(p, n, c); p.end();
    cache.insert(key, pm);
    return pm;
}
static QPixmap logoPix(int sz) {
    QPixmap pm(sz * 2, sz * 2); pm.fill(Qt::transparent); pm.setDevicePixelRatio(2);
    QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen); p.setBrush(g_t.accent);
    p.drawRoundedRect(QRectF(0, 0, sz, sz), sz * 0.3, sz * 0.3);
    p.drawPixmap(QPointF(sz * 0.2, sz * 0.2), iconPix("wind", int(sz * 0.6), QColor(Qt::white)));
    return pm;
}

// ---------- genel yardımcılar ----------
static QString fmtBytes(qint64 b) {
    if (b < 0) return QStringLiteral("Unavailable");
    double g = b / 1073741824.0;
    return g >= 1.0 ? QString::number(g, 'f', 1) + " GB" : QString::number(b / 1048576.0, 'f', 0) + " MB";
}
static QLabel *mkLabel(const QString &t, const char *role = nullptr, QWidget *p = nullptr) {
    auto *l = new QLabel(t, p);
    if (role) l->setProperty("role", QString::fromLatin1(role));
    return l;
}
static void fadeWidget(QWidget *w, bool in, int ms = 180) {
    if (!w) return;
    if (!Cfg::anim()) { w->setVisible(in); return; }
    if (auto *old = w->findChild<QPropertyAnimation *>(QStringLiteral("fadeAnim"))) { old->setObjectName(QString()); old->stop(); old->deleteLater(); }
    auto *e = qobject_cast<QGraphicsOpacityEffect *>(w->graphicsEffect());
    if (!e) { e = new QGraphicsOpacityEffect(w); e->setOpacity(in ? 0.0 : 1.0); w->setGraphicsEffect(e); }
    auto *a = new QPropertyAnimation(e, "opacity", w);
    a->setObjectName(QStringLiteral("fadeAnim")); a->setDuration(ms);
    a->setStartValue(e->opacity()); a->setEndValue(in ? 1.0 : 0.0);
    if (in) w->show();
    QObject::connect(a, &QPropertyAnimation::finished, w, [w, in, a] {
        if (a->currentTime() < a->duration()) return;
        if (in) QTimer::singleShot(0, w, [w] { w->setGraphicsEffect(nullptr); });
        else w->hide();
    });
    a->start(QAbstractAnimation::DeleteWhenStopped);
}
static void toast(QWidget *host, const QString &msg, bool err = false) {
    if (!host) return;
    host = host->window();
    auto *l = new QLabel(msg, host);
    l->setWordWrap(true); l->setAlignment(Qt::AlignCenter); l->setMaximumWidth(520); l->setAttribute(Qt::WA_TransparentForMouseEvents);
    QColor bgc = err ? g_t.danger : (g_t.dark ? C("#ECECEC") : C("#202020"));
    QColor fgc = err ? C("#FFFFFF") : (g_t.dark ? C("#202020") : C("#FFFFFF"));
    l->setStyleSheet(QStringLiteral("background:%1;color:%2;border-radius:12px;padding:10px 16px;font-size:13px;").arg(bgc.name(), fgc.name()));
    l->adjustSize();
    l->move((host->width() - l->width()) / 2, host->height() - l->height() - 130);
    l->raise(); fadeWidget(l, true, 160);
    QTimer::singleShot(4200, l, [l] { fadeWidget(l, false, 220); QTimer::singleShot(300, l, &QObject::deleteLater); });
}

// ---------- genel buton ----------
class FlatButton : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal hoverT READ hoverT WRITE setHoverT)
public:
    enum Kind { Ghost, Outline, Filled, Raised };
    explicit FlatButton(const QString &icon = QString(), const QString &text = QString(), QWidget *p = nullptr) : QAbstractButton(p), m_icon(icon) {
        setText(text); setCursor(Qt::PointingHandCursor); setFocusPolicy(Qt::NoFocus);
        m_anim = new QPropertyAnimation(this, "hoverT", this); m_anim->setDuration(150);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
    qreal hoverT() const { return m_h; }
    void setHoverT(qreal v) { m_h = v; update(); }
    FlatButton *kind(Kind k) { m_kind = k; update(); return this; }
    FlatButton *setIconName(const QString &n) { m_icon = n; update(); return this; }
    FlatButton *isz(int s) { m_isz = s; return this; }
    FlatButton *rad(int r) { m_rad = r; return this; }
    FlatButton *leftAlign(bool b = true) { m_left = b; return this; }
    FlatButton *strong(bool b = true) { m_strong = b; update(); return this; }
    FlatButton *trail(const QString &n) { m_trail = n; return this; }
    FlatButton *tint(const QColor &c) { m_tint = c; update(); return this; }
    FlatButton *fixed(int w, int h) { setFixedSize(w, h); return this; }
    FlatButton *tip(const QString &t) { setToolTip(t); return this; }
    QSize sizeHint() const override { return QSize(contentW() + 24, 36); }
protected:
    void enterEvent(QEnterEvent *e) override { QAbstractButton::enterEvent(e); animTo(1); }
    void leaveEvent(QEvent *e) override { QAbstractButton::leaveEvent(e); animTo(0); }
    void animTo(qreal t) {
        m_anim->stop();
        if (!Cfg::anim()) { setHoverT(t); return; }
        m_anim->setStartValue(m_h); m_anim->setEndValue(t); m_anim->start();
    }
    int contentW() const {
        QFontMetrics fm(font());
        int w = fm.horizontalAdvance(text());
        if (!m_icon.isEmpty()) w += m_isz + (text().isEmpty() ? 0 : 10);
        if (!m_trail.isEmpty()) w += 20;
        return w;
    }
    QColor fg() const {
        QColor f = m_kind == Filled ? QColor(Qt::white) : (m_tint.isValid() ? m_tint : ((m_strong || m_kind == Raised) ? g_t.text : g_t.text2));
        if (!isEnabled() && m_kind != Filled) f.setAlphaF(0.45);
        return f;
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        qreal rd = m_rad < 0 ? qMin(r.width(), r.height()) / 2 : m_rad;
        QColor bg(Qt::transparent);
        if (m_kind == Filled) bg = isEnabled() ? blendC(g_t.accent, g_t.accentHi, m_h) : (g_t.dark ? C("#4A4A4A") : C("#D2D2D2"));
        else if (m_kind == Raised) bg = blendC(g_t.bg, g_t.hover, m_h);
        else { bg = isChecked() ? g_t.sel : g_t.hover; if (!isChecked()) bg.setAlphaF(m_h); }
        p.setPen(Qt::NoPen); p.setBrush(bg); p.drawRoundedRect(r, rd, rd);
        if (m_kind == Outline || m_kind == Raised) { p.setBrush(Qt::NoBrush); p.setPen(QPen(g_t.border, 1)); p.drawRoundedRect(r, rd, rd); }
        QColor f = fg(); QFontMetrics fm(font());
        int x = m_left ? 10 : qMax(0, (width() - contentW()) / 2);
        if (!m_icon.isEmpty()) { p.drawPixmap(x, (height() - m_isz) / 2, iconPix(m_icon, m_isz, f)); x += m_isz + (text().isEmpty() ? 0 : 10); }
        if (!text().isEmpty()) {
            int avail = width() - x - (m_left ? 10 : 6) - (m_trail.isEmpty() ? 0 : 20);
            QString el = fm.elidedText(text(), Qt::ElideRight, qMax(10, avail));
            p.setPen(f); p.setFont(font()); p.drawText(QRect(x, 0, qMax(10, avail), height()), Qt::AlignVCenter | Qt::AlignLeft, el);
            x += fm.horizontalAdvance(el) + 4;
        }
        if (!m_trail.isEmpty()) p.drawPixmap(x, (height() - 16) / 2, iconPix(m_trail, 16, f));
    }
    QString m_icon, m_trail; int m_isz = 20, m_rad = 10; bool m_left = false, m_strong = false; Kind m_kind = Ghost;
    QColor m_tint; qreal m_h = 0; QPropertyAnimation *m_anim = nullptr;
};
static FlatButton *fb(const QString &icon, const QString &text = QString(), QWidget *p = nullptr) { return new FlatButton(icon, text, p); }

// ---------- animasyonlu açılır panel (hesap menüsü, model seçici, düşünme seçici) ----------
class PopupPanel : public QFrame {
    Q_OBJECT
public:
    static constexpr int M = 14;
    explicit PopupPanel(QWidget *parent = nullptr) : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) {
        setAttribute(Qt::WA_TranslucentBackground);
        m_lay = new QVBoxLayout(this);
        m_lay->setContentsMargins(M + 6, M + 6, M + 6, M + 6); m_lay->setSpacing(2);
        m_lay->setSizeConstraint(QLayout::SetNoConstraint);
    }
    QVBoxLayout *body() { return m_lay; }
    void showNear(QWidget *anchor, bool above, bool alignRight = false) {
        m_lay->activate(); adjustSize();
        QPoint ga = anchor->mapToGlobal(QPoint(0, 0));
        int x = alignRight ? ga.x() + anchor->width() - width() + M : ga.x() - M;
        int y = above ? ga.y() - height() + M - 6 : ga.y() + anchor->height() - M + 6;
        QRect av = anchor->screen()->availableGeometry();
        x = qBound(av.left(), x, qMax(av.left(), av.right() - width()));
        y = qBound(av.top(), y, qMax(av.top(), av.bottom() - height()));
        QRect fin(x, y, width(), height());
        if (Cfg::anim()) {
            QRect st(fin.x() + int(fin.width() * 0.03), above ? fin.y() + int(fin.height() * 0.06) : fin.y(), int(fin.width() * 0.94), int(fin.height() * 0.94));
            setWindowOpacity(0.0); setGeometry(st); show();
            auto *g = new QParallelAnimationGroup(this);
            auto *a1 = new QPropertyAnimation(this, "geometry"); a1->setDuration(150); a1->setStartValue(st); a1->setEndValue(fin); a1->setEasingCurve(QEasingCurve::OutCubic);
            auto *a2 = new QPropertyAnimation(this, "windowOpacity"); a2->setDuration(150); a2->setStartValue(0.0); a2->setEndValue(1.0);
            g->addAnimation(a1); g->addAnimation(a2); g->start(QAbstractAnimation::DeleteWhenStopped);
        } else { setWindowOpacity(1.0); setGeometry(fin); show(); }
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QRectF r = QRectF(rect()).adjusted(M, M, -M, -M);
        p.setPen(Qt::NoPen);
        for (int i = 6; i >= 1; --i) { p.setBrush(QColor(0, 0, 0, g_t.dark ? 22 : 12)); p.drawRoundedRect(r.adjusted(-i * 1.6, -i * 1.6 + 3, i * 1.6, i * 1.6 + 3), 16 + i, 16 + i); }
        p.setBrush(g_t.dark ? g_t.composer : g_t.bg); p.setPen(QPen(g_t.border, 1)); p.drawRoundedRect(r, 14, 14);
    }
    QVBoxLayout *m_lay;
};

// ---------- menü satırı ----------
class MenuRow : public QAbstractButton {
    Q_OBJECT
public:
    MenuRow(const QString &icon, const QString &title, const QString &sub = QString(), QWidget *p = nullptr) : QAbstractButton(p), m_icon(icon), m_title(title), m_sub(sub) {
        setCursor(Qt::PointingHandCursor); setFocusPolicy(Qt::NoFocus);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
    MenuRow *check(bool b) { m_check = b; update(); return this; }
    MenuRow *badge(const QString &t) { m_badge = t; update(); return this; }
    MenuRow *danger(bool d = true) { m_danger = d; update(); return this; }
    QSize sizeHint() const override { return QSize(240, m_sub.isEmpty() ? 38 : 56); }
    QSize minimumSizeHint() const override { return QSize(120, sizeHint().height()); }
signals:
    void hovered();
protected:
    void enterEvent(QEnterEvent *e) override { QAbstractButton::enterEvent(e); emit hovered(); update(); }
    void leaveEvent(QEvent *e) override { QAbstractButton::leaveEvent(e); update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        if (underMouse()) { p.setPen(Qt::NoPen); p.setBrush(g_t.hover); p.drawRoundedRect(QRectF(rect()).adjusted(0, 1, 0, -1), 8, 8); }
        QColor fg = m_danger ? g_t.danger : g_t.text; int x = 12;
        if (!m_icon.isEmpty()) { p.drawPixmap(x, (height() - 20) / 2, iconPix(m_icon, 20, fg)); x += 30; }
        int right = width() - 12;
        if (m_check) { p.drawPixmap(right - 18, (height() - 18) / 2, iconPix("check", 18, g_t.accent)); right -= 26; }
        if (!m_badge.isEmpty()) {
            QFont f = font(); f.setPixelSize(11); QFontMetrics fm(f); int bw = fm.horizontalAdvance(m_badge) + 14;
            QRect br(right - bw, (height() - 20) / 2, bw, 20);
            p.setPen(Qt::NoPen); p.setBrush(g_t.hover); p.drawRoundedRect(br, 10, 10); p.setFont(f); p.setPen(g_t.text2); p.drawText(br, Qt::AlignCenter, m_badge);
            right -= bw + 8;
        }
        int avail = qMax(20, right - x);
        QFont tf = font(); tf.setPixelSize(14);
        if (m_sub.isEmpty()) {
            QFontMetrics tfm(tf); p.setFont(tf); p.setPen(fg);
            p.drawText(QRect(x, 0, avail, height()), Qt::AlignVCenter | Qt::AlignLeft, tfm.elidedText(m_title, Qt::ElideRight, avail));
        } else {
            tf.setWeight(QFont::Medium); QFontMetrics tfm(tf); p.setFont(tf); p.setPen(fg);
            p.drawText(QRect(x, 8, avail, 20), Qt::AlignVCenter | Qt::AlignLeft, tfm.elidedText(m_title, Qt::ElideRight, avail));
            QFont sf = font(); sf.setPixelSize(12); QFontMetrics sfm(sf); p.setFont(sf); p.setPen(g_t.text2);
            p.drawText(QRect(x, 29, avail, 18), Qt::AlignVCenter | Qt::AlignLeft, sfm.elidedText(m_sub, Qt::ElideRight, avail));
        }
    }
    QString m_icon, m_title, m_sub, m_badge; bool m_check = false, m_danger = false;
};

// ---------- küçük widget'lar ----------
class TypingIndicator : public QWidget {
    Q_OBJECT
public:
    explicit TypingIndicator(QWidget *p = nullptr) : QWidget(p) {
        setFixedSize(46, 24);
        a = new QVariantAnimation(this); a->setStartValue(0.0); a->setEndValue(1.0); a->setDuration(1100); a->setLoopCount(-1);
        connect(a, &QVariantAnimation::valueChanged, this, [this] { update(); });
    }
protected:
    void showEvent(QShowEvent *) override { if (Cfg::anim()) a->start(); }
    void hideEvent(QHideEvent *) override { a->stop(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        qreal t = a->state() == QAbstractAnimation::Running ? a->currentValue().toReal() : 0.5;
        for (int i = 0; i < 3; ++i) {
            qreal ph = std::fmod(t - i * 0.18 + 1.0, 1.0), s = 0.5 + 0.5 * std::sin(ph * 2 * kPi);
            QColor c = g_t.text2; c.setAlphaF(0.35 + 0.65 * s);
            p.setPen(Qt::NoPen); p.setBrush(c); p.drawEllipse(QPointF(10 + 13 * i, 12), 3.2 + 1.2 * s, 3.2 + 1.2 * s);
        }
    }
    QVariantAnimation *a;
};
class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(int sz, QWidget *p = nullptr) : QWidget(p) { setFixedSize(sz, sz); }
    void setInitials(const QString &s) { ini = s; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen); p.setBrush(C("#8A93A3")); p.drawEllipse(rect());
        QFont f = font(); f.setPixelSize(int(height() * 0.38)); f.setWeight(QFont::DemiBold); p.setFont(f); p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, ini);
    }
    QString ini;
};
class LogoLabel : public QLabel {
    Q_OBJECT
public:
    explicit LogoLabel(int sz, QWidget *p = nullptr) : QLabel(p), s(sz) {
        setFixedSize(sz, sz); setPixmap(logoPix(sz));
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { setPixmap(logoPix(s)); });
    }
    int s;
};
class RoundImage : public QWidget {
    Q_OBJECT
public:
    RoundImage(const QPixmap &pm, const QSize &sz, int r, const QString &lbl, QWidget *p = nullptr) : QWidget(p), m_pm(pm), m_r(r), m_l(lbl) { setFixedSize(sz); }
    void setClickable(bool c) { setCursor(c ? Qt::PointingHandCursor : Qt::ArrowCursor); m_click = c; }
signals:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent *e) override { if (m_click && e->button() == Qt::LeftButton && rect().contains(e->position().toPoint())) emit clicked(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path; path.addRoundedRect(QRectF(rect()), m_r, m_r); p.setClipPath(path);
        if (m_pm.isNull()) {
            p.fillRect(rect(), g_t.hover); p.setPen(g_t.text2); QFont f = font(); f.setPixelSize(11); p.setFont(f);
            p.drawText(rect().adjusted(4, 4, -4, -4), Qt::AlignCenter | Qt::TextWordWrap, m_l);
        } else p.drawPixmap(rect(), m_pm);
        p.setClipping(false); p.setBrush(Qt::NoBrush); p.setPen(QPen(g_t.border, 1));
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), m_r, m_r);
    }
    QPixmap m_pm; int m_r; QString m_l; bool m_click = false;
};
static QPixmap thumbPix(const QString &path, int sz) {
    QImageReader rd(path); rd.setAutoTransform(true); QImage img = rd.read();
    if (img.isNull()) return QPixmap();
    img = img.scaled(sz * 2, sz * 2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    img = img.copy((img.width() - sz * 2) / 2, (img.height() - sz * 2) / 2, sz * 2, sz * 2);
    QPixmap pm = QPixmap::fromImage(img); pm.setDevicePixelRatio(2); return pm;
}

// ---------- kaydırma alanı (yumuşak kaydırma + alta yapışma) ----------
class SmoothScroll : public QScrollArea {
    Q_OBJECT
public:
    explicit SmoothScroll(QWidget *p = nullptr) : QScrollArea(p) {
        setWidgetResizable(true); setFrameShape(QFrame::NoFrame); setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_anim = new QPropertyAnimation(verticalScrollBar(), "value", this); m_anim->setEasingCurve(QEasingCurve::OutCubic); m_anim->setDuration(220);
        connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this] { if (m_stick) setBottom(); report(); });
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) { if (!m_prog) m_stick = v >= verticalScrollBar()->maximum() - 40; report(); });
    }
    void setStick(bool s) { m_stick = s; if (s) setBottom(); }
    void scrollToBottom(bool animated) {
        m_stick = true; QScrollBar *sb = verticalScrollBar();
        if (!animated || !Cfg::anim()) { setBottom(); return; }
        m_anim->stop(); m_anim->setStartValue(sb->value()); m_anim->setEndValue(sb->maximum()); m_anim->start();
    }
signals:
    void bottomState(bool atBottom);
protected:
    void setBottom() { m_prog = true; verticalScrollBar()->setValue(verticalScrollBar()->maximum()); m_prog = false; }
    void report() { bool b = verticalScrollBar()->maximum() - verticalScrollBar()->value() < 120; if (b != m_last) { m_last = b; emit bottomState(b); } }
    void wheelEvent(QWheelEvent *e) override {
        if (!Cfg::anim() || e->angleDelta().y() == 0 || !e->pixelDelta().isNull()) { QScrollArea::wheelEvent(e); return; }
        QScrollBar *sb = verticalScrollBar();
        int base = m_anim->state() == QAbstractAnimation::Running ? m_anim->endValue().toInt() : sb->value();
        int tgt = qBound(sb->minimum(), base - e->angleDelta().y(), sb->maximum());
        m_stick = false;
        m_anim->stop(); m_anim->setStartValue(sb->value()); m_anim->setEndValue(tgt); m_anim->start(); e->accept();
    }
    QPropertyAnimation *m_anim; bool m_stick = true, m_prog = false, m_last = true;
};

// ---------- CSS ----------
static QString buildCss() {
    QString c = QString::fromUtf8(R"CSS(
QWidget{color:$tx$;font-family:"Segoe UI","Segoe UI Variable Text",Arial,sans-serif;font-size:14px;}
QDialog,QMainWindow{background:$bg$;}
QToolTip{background:$tx$;color:$bg$;border:none;padding:5px 9px;border-radius:6px;}
QLabel{background:transparent;}
QLabel[role="muted"]{color:$tx2$;}
QLabel[role="small"]{color:$tx2$;font-size:12px;}
QLabel[role="h1"]{font-size:24px;font-weight:600;}
QLabel[role="h2"]{font-size:16px;font-weight:600;}
QLabel[role="brand"]{font-size:19px;font-weight:600;}
QLabel[role="err"]{color:$dg$;font-size:13px;}
QLabel[role="ok"]{color:$ok$;font-size:13px;}
QLabel[role="msg"]{font-size:15px;}
QLineEdit,QComboBox,QSpinBox{background:$bg$;border:1px solid $bd$;border-radius:8px;padding:7px 10px;selection-background-color:$ac$;selection-color:#FFFFFF;}
QLineEdit:focus,QComboBox:focus,QSpinBox:focus{border:1px solid $ac$;}
QLineEdit:disabled,QComboBox:disabled{color:$tx2$;}
QComboBox::drop-down{border:none;width:26px;}
QComboBox::down-arrow{image:url("$chev$");width:12px;height:12px;}
QComboBox QAbstractItemView{background:$bg$;border:1px solid $bd$;selection-background-color:$hv$;selection-color:$tx$;outline:0;}
QTextEdit#composerEdit{background:transparent;border:none;padding:0;font-size:15px;selection-background-color:$ac$;selection-color:#FFFFFF;}
QTextEdit#field{background:$bg$;border:1px solid $bd$;border-radius:8px;padding:6px 8px;}
QPlainTextEdit#codeBody{background:transparent;border:none;font-family:Consolas,"Cascadia Mono","Courier New",monospace;font-size:13px;selection-background-color:$ac$;selection-color:#FFFFFF;}
QFrame#codeBox{background:$cb$;border:1px solid $bd$;border-radius:10px;}
QFrame#codeHead{background:$ch$;border:none;border-top-left-radius:9px;border-top-right-radius:9px;}
QFrame#bubble{background:$bub$;border:none;border-radius:18px;}
QFrame#authCard{background:$bg$;border:1px solid $bd$;border-radius:16px;}
QFrame#errBox{background:transparent;border:1px solid $dg$;border-radius:10px;}
QFrame#panelBox{background:transparent;border:1px solid $bd$;border-radius:12px;}
QFrame#hline,QFrame#sbLine{background:$bd$;border:none;}
QPushButton{background:$bg$;border:1px solid $bd$;border-radius:10px;padding:8px 16px;}
QPushButton:hover{background:$hv$;}
QPushButton:disabled{color:$tx2$;}
QPushButton[kind="primary"]{background:$ac$;color:#FFFFFF;border:none;}
QPushButton[kind="primary"]:hover{background:$ach$;}
QPushButton[kind="primary"]:disabled{background:$hv$;color:$tx2$;}
QPushButton[kind="danger"]{color:$dg$;border:1px solid $dg$;}
QScrollArea{border:none;background:transparent;}
QScrollBar:vertical{background:transparent;width:12px;margin:2px 0;}
QScrollBar::handle:vertical{background:$sbh$;border-radius:4px;min-height:40px;margin:0 2px;}
QScrollBar::handle:vertical:hover{background:$sbhh$;}
QScrollBar:horizontal{background:transparent;height:12px;margin:0 2px;}
QScrollBar::handle:horizontal{background:$sbh$;border-radius:4px;min-width:40px;margin:2px 0;}
QScrollBar::handle:horizontal:hover{background:$sbhh$;}
QScrollBar::add-line,QScrollBar::sub-line{width:0;height:0;border:none;background:none;}
QScrollBar::add-page,QScrollBar::sub-page{background:none;}
QMenu{background:$bg$;border:1px solid $bd$;padding:6px;}
QMenu::item{padding:8px 30px 8px 14px;border-radius:6px;margin:1px 0;}
QMenu::item:selected{background:$hv$;color:$tx$;}
QMenu::separator{height:1px;background:$bd$;margin:5px 8px;}
QCheckBox{spacing:9px;}
QCheckBox::indicator{width:16px;height:16px;border:1px solid $bd$;border-radius:4px;background:$bg$;}
QCheckBox::indicator:checked{background:$ac$;border:1px solid $ac$;image:url("$chk$");}
QRadioButton{spacing:9px;}
QListWidget{background:transparent;border:none;outline:0;}
QListWidget::item{padding:9px 10px;border-radius:8px;margin:1px 0;}
QListWidget::item:hover{background:$hv$;}
QListWidget::item:selected{background:$sel$;color:$tx$;}
QProgressBar{background:$hv$;border:none;border-radius:4px;max-height:8px;min-height:8px;color:transparent;}
QProgressBar::chunk{background:$ac$;border-radius:4px;}
)CSS");
    const QHash<QString, QString> m{
        {"$bg$", g_t.bg.name()}, {"$tx$", g_t.text.name()}, {"$tx2$", g_t.text2.name()}, {"$bd$", g_t.border.name()}, {"$hv$", g_t.hover.name()},
        {"$sel$", g_t.sel.name()}, {"$ac$", g_t.accent.name()}, {"$ach$", g_t.accentHi.name()}, {"$dg$", g_t.danger.name()}, {"$ok$", g_t.ok.name()},
        {"$cb$", g_t.codeBg.name()}, {"$ch$", g_t.codeHead.name()}, {"$bub$", g_t.bubble.name()}, {"$chk$", g_chk}, {"$chev$", g_chev},
        {"$sbh$", blendC(g_t.bg, g_t.text, 0.22).name()}, {"$sbhh$", blendC(g_t.bg, g_t.text, 0.38).name()}};
    for (auto it = m.cbegin(); it != m.cend(); ++it) c.replace(it.key(), it.value());
    return c;
}
static bool systemDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#elif defined(Q_OS_WIN)
    QSettings r(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
    return r.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    return false;
#endif
}
static void applyTheme() {
    int m = Cfg::get("theme", 0).toInt();
    g_t = makeTheme(m == 1 || (m == 2 && systemDark()));
    const QString tmp = QDir::tempPath();
    g_chk = QDir::fromNativeSeparators(tmp + "/ruzgar_check.png");
    g_chev = QDir::fromNativeSeparators(tmp + (g_t.dark ? "/ruzgar_chev_d.png" : "/ruzgar_chev_l.png"));
    {
        QImage img(14, 14, QImage::Format_ARGB32_Premultiplied); img.fill(Qt::transparent);
        QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF(QList<QPointF>{{3, 7.5}, {6, 10.5}, {11, 4}})); p.end(); img.save(g_chk, "PNG");
        QImage im2(12, 12, QImage::Format_ARGB32_Premultiplied); im2.fill(Qt::transparent);
        QPainter p2(&im2); p2.setRenderHint(QPainter::Antialiasing); p2.setPen(QPen(g_t.text2, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p2.drawPolyline(QPolygonF(QList<QPointF>{{2.5, 4.5}, {6, 8}, {9.5, 4.5}})); p2.end(); im2.save(g_chev, "PNG");
    }
    QPalette pal;
    pal.setColor(QPalette::Window, g_t.bg); pal.setColor(QPalette::WindowText, g_t.text); pal.setColor(QPalette::Base, g_t.bg);
    pal.setColor(QPalette::AlternateBase, g_t.sidebar); pal.setColor(QPalette::Text, g_t.text); pal.setColor(QPalette::Button, g_t.bg);
    pal.setColor(QPalette::ButtonText, g_t.text); pal.setColor(QPalette::ToolTipBase, g_t.text); pal.setColor(QPalette::ToolTipText, g_t.bg);
    pal.setColor(QPalette::Highlight, g_t.accent); pal.setColor(QPalette::HighlightedText, Qt::white); pal.setColor(QPalette::PlaceholderText, g_t.text2);
    pal.setColor(QPalette::Link, g_t.accent);
    pal.setColor(QPalette::Disabled, QPalette::Text, g_t.text2); pal.setColor(QPalette::Disabled, QPalette::ButtonText, g_t.text2);
    qApp->setPalette(pal);
    qApp->setStyleSheet(buildCss());
    emit ThemeBus::I().changed();
}
static void switchTheme(QWidget *host) {
    QLabel *ov = nullptr;
    if (host && Cfg::anim() && host->isVisible()) {
        ov = new QLabel(host); ov->setPixmap(host->grab()); ov->setGeometry(host->rect());
        ov->setAttribute(Qt::WA_TransparentForMouseEvents); ov->show(); ov->raise();
    }
    applyTheme();
    if (ov) {
        auto *e = new QGraphicsOpacityEffect(ov); ov->setGraphicsEffect(e);
        auto *a = new QPropertyAnimation(e, "opacity", ov); a->setDuration(320); a->setStartValue(1.0); a->setEndValue(0.0);
        QObject::connect(a, &QPropertyAnimation::finished, ov, &QObject::deleteLater);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

// ---------- Windows Kimlik Bilgisi Yöneticisi (API anahtarları) ----------
struct SecretStore {
    static std::wstring target(const QString &id) { return (QStringLiteral("RuzgarAI/") + id).toStdWString(); }
    static bool save(const QString &id, const QString &secret) {
#ifdef Q_OS_WIN
        std::wstring tn = target(id);
        if (secret.isEmpty()) { CredDeleteW(tn.c_str(), CRED_TYPE_GENERIC, 0); return true; }
        QByteArray blob = secret.toUtf8(); std::wstring un = L"RuzgarAI";
        CREDENTIALW c; ZeroMemory(&c, sizeof(c));
        c.Type = CRED_TYPE_GENERIC; c.TargetName = const_cast<LPWSTR>(tn.c_str()); c.UserName = const_cast<LPWSTR>(un.c_str());
        c.CredentialBlobSize = DWORD(blob.size()); c.CredentialBlob = reinterpret_cast<LPBYTE>(blob.data()); c.Persist = CRED_PERSIST_LOCAL_MACHINE;
        return CredWriteW(&c, 0) == TRUE;
#else
        Q_UNUSED(id) Q_UNUSED(secret)
        return false;
#endif
    }
    static QString load(const QString &id) {
#ifdef Q_OS_WIN
        std::wstring tn = target(id); PCREDENTIALW pc = nullptr;
        if (CredReadW(tn.c_str(), CRED_TYPE_GENERIC, 0, &pc) && pc) {
            QString s = QString::fromUtf8(reinterpret_cast<const char *>(pc->CredentialBlob), int(pc->CredentialBlobSize));
            CredFree(pc); return s;
        }
#else
        Q_UNUSED(id)
#endif
        return QString();
    }
};

// ============================================================================
// 2) Yardımcılar, veritabanı, hesap ve sohbet yöneticileri
// ============================================================================
static QVBoxLayout *vb(QWidget *w, int m = 0, int sp = 0) { auto *l = new QVBoxLayout(w); l->setContentsMargins(m, m, m, m); l->setSpacing(sp); return l; }
static QHBoxLayout *hb(QWidget *w, int m = 0, int sp = 0) { auto *l = new QHBoxLayout(w); l->setContentsMargins(m, m, m, m); l->setSpacing(sp); return l; }
static qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }
static QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(16); }
static QNetworkAccessManager *nam() { static QNetworkAccessManager *n = new QNetworkAccessManager(qApp); return n; }

class Db {
public:
    static bool open(QString *err) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
        d.setDatabaseName(dir + QStringLiteral("/ruzgar.db"));
        if (!d.open()) { if (err) *err = d.lastError().text(); return false; }
        QSqlQuery q(d);
        q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
        const QStringList ddl = {
            "CREATE TABLE IF NOT EXISTS users(id TEXT PRIMARY KEY, first TEXT, last TEXT, username TEXT UNIQUE COLLATE NOCASE, email TEXT UNIQUE COLLATE NOCASE, salt BLOB, hash BLOB, iters INTEGER, initials TEXT, instructions TEXT DEFAULT '', created INTEGER)",
            "CREATE TABLE IF NOT EXISTS conversations(id TEXT PRIMARY KEY, user_id TEXT, title TEXT, created INTEGER, updated INTEGER, pinned INTEGER DEFAULT 0, project TEXT DEFAULT '', model TEXT DEFAULT '')",
            "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY AUTOINCREMENT, conv_id TEXT, role TEXT, content TEXT, ts INTEGER, images TEXT DEFAULT '[]', model TEXT DEFAULT '', feedback INTEGER DEFAULT 0)",
            "CREATE INDEX IF NOT EXISTS idx_msg_conv ON messages(conv_id, id)",
            "CREATE INDEX IF NOT EXISTS idx_conv_user ON conversations(user_id, updated)"};
        for (const QString &s : ddl) if (!q.exec(s)) { if (err) *err = q.lastError().text(); return false; }
        return true;
    }
    static bool run(QSqlQuery &q, const char *what) {
        if (q.exec()) return true;
        emit ThemeBus::I().error(QStringLiteral("Veritabanı hatası (%1): %2").arg(QString::fromUtf8(what), q.lastError().text()));
        return false;
    }
};

// ---------- hesaplar ----------
struct UserRec {
    QString id, first, last, username, email, initials, instructions; qint64 created = 0;
    QString fullName() const { return (first + QLatin1Char(' ') + last).trimmed(); }
};
class UserManager : public QObject {
    Q_OBJECT
public:
    static constexpr int kIters = 150000;
    explicit UserManager(QObject *p = nullptr) : QObject(p) {}
    bool hasAccounts() const { QSqlQuery q(QStringLiteral("SELECT COUNT(*) FROM users")); return q.next() && q.value(0).toInt() > 0; }
    bool loggedIn() const { return !m_u.id.isEmpty(); }
    const UserRec &user() const { return m_u; }
    static QString initialsOf(const QString &f, const QString &l) {
        QString s; if (!f.isEmpty()) s += f.at(0); if (!l.isEmpty()) s += l.at(0);
        return QLocale(QLocale::Turkish).toUpper(s);
    }
    QString registerUser(QString f, QString l, QString un, QString em, const QString &pw, const QString &pw2) {
        f = f.trimmed(); l = l.trimmed(); un = un.trimmed(); em = em.trimmed();
        if (f.isEmpty() || l.isEmpty()) return QStringLiteral("Ad ve soyad zorunludur.");
        static const QRegularExpression unRe(QStringLiteral("^[A-Za-z0-9_.-]{3,32}$"));
        if (!unRe.match(un).hasMatch()) return QStringLiteral("Kullanıcı adı 3–32 karakter olmalı (harf, rakam, _ . -).");
        static const QRegularExpression emRe(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        if (!emRe.match(em).hasMatch()) return QStringLiteral("Geçerli bir e-posta adresi girin.");
        if (pw.size() < 8) return QStringLiteral("Parola en az 8 karakter olmalı.");
        if (pw != pw2) return QStringLiteral("Parolalar eşleşmiyor.");
        QSqlQuery c; c.prepare(QStringLiteral("SELECT 1 FROM users WHERE username=? OR email=?")); c.addBindValue(un); c.addBindValue(em);
        if (!Db::run(c, "kayıt")) return QStringLiteral("Veritabanı hatası: hesap kontrol edilemedi.");
        if (c.next()) return QStringLiteral("Bu kullanıcı adı veya e-posta zaten kayıtlı.");
        const QByteArray salt = randomBytes(); const QByteArray h = hashPw(pw, salt, kIters);
        const QString id = newId();
        QSqlQuery q; q.prepare(QStringLiteral("INSERT INTO users(id,first,last,username,email,salt,hash,iters,initials,created) VALUES(?,?,?,?,?,?,?,?,?,?)"));
        q.addBindValue(id); q.addBindValue(f); q.addBindValue(l); q.addBindValue(un); q.addBindValue(em); q.addBindValue(salt); q.addBindValue(h);
        q.addBindValue(kIters); q.addBindValue(initialsOf(f, l)); q.addBindValue(nowMs());
        if (!Db::run(q, "kayıt")) return QStringLiteral("Hesap oluşturulamadı (veritabanı hatası).");
        setCurrent(id);
        return QString();
    }
    QString login(const QString &idOrEmail, const QString &pw) {
        const qint64 now = nowMs();
        if (now < m_lockUntil) return QStringLiteral("Çok fazla başarısız deneme. %1 sn sonra tekrar deneyin.").arg((m_lockUntil - now) / 1000 + 1);
        const QString k = idOrEmail.trimmed();
        if (k.isEmpty() || pw.isEmpty()) return QStringLiteral("E-posta/kullanıcı adı ve parola girin.");
        QSqlQuery q; q.prepare(QStringLiteral("SELECT id,salt,hash,iters FROM users WHERE username=? OR email=?")); q.addBindValue(k); q.addBindValue(k);
        if (!Db::run(q, "giriş")) return QStringLiteral("Veritabanı hatası: giriş yapılamadı.");
        bool ok = false; QString id;
        if (q.next()) {
            id = q.value(0).toString();
            ok = ctEq(hashPw(pw, q.value(1).toByteArray(), q.value(3).toInt()), q.value(2).toByteArray());
        } else { hashPw(pw, randomBytes(), kIters); }  // zamanlama farkını azalt
        if (!ok) {
            if (++m_fails >= 5) { m_lockUntil = nowMs() + 30000; m_fails = 0; }
            return QStringLiteral("E-posta/kullanıcı adı veya parola hatalı.");
        }
        m_fails = 0; setCurrent(id);
        return QString();
    }
    bool tryAutoLogin() {
        if (!Cfg::get("keepSignedIn", true).toBool()) return false;
        const QString id = Cfg::get("lastUser").toString(); if (id.isEmpty()) return false;
        QSqlQuery q; q.prepare(QStringLiteral("SELECT 1 FROM users WHERE id=?")); q.addBindValue(id);
        if (!q.exec() || !q.next()) return false;
        setCurrent(id); return true;
    }
    void logout() { m_u = UserRec(); Cfg::set("lastUser", QString()); emit userChanged(); }
    QString updateProfile(QString f, QString l, QString un, QString em, QString initials) {
        f = f.trimmed(); l = l.trimmed(); un = un.trimmed(); em = em.trimmed(); initials = initials.trimmed();
        if (f.isEmpty() || l.isEmpty()) return QStringLiteral("Ad ve soyad zorunludur.");
        static const QRegularExpression unRe(QStringLiteral("^[A-Za-z0-9_.-]{3,32}$"));
        if (!unRe.match(un).hasMatch()) return QStringLiteral("Kullanıcı adı 3–32 karakter olmalı (harf, rakam, _ . -).");
        static const QRegularExpression emRe(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        if (!emRe.match(em).hasMatch()) return QStringLiteral("Geçerli bir e-posta adresi girin.");
        QSqlQuery c; c.prepare(QStringLiteral("SELECT 1 FROM users WHERE (username=? OR email=?) AND id<>?")); c.addBindValue(un); c.addBindValue(em); c.addBindValue(m_u.id);
        if (!Db::run(c, "profil")) return QStringLiteral("Veritabanı hatası.");
        if (c.next()) return QStringLiteral("Bu kullanıcı adı veya e-posta başka bir hesapta kayıtlı.");
        if (initials.isEmpty()) initials = initialsOf(f, l); else initials = QLocale(QLocale::Turkish).toUpper(initials.left(2));
        QSqlQuery q; q.prepare(QStringLiteral("UPDATE users SET first=?,last=?,username=?,email=?,initials=? WHERE id=?"));
        q.addBindValue(f); q.addBindValue(l); q.addBindValue(un); q.addBindValue(em); q.addBindValue(initials); q.addBindValue(m_u.id);
        if (!Db::run(q, "profil")) return QStringLiteral("Profil kaydedilemedi.");
        setCurrent(m_u.id); return QString();
    }
    QString setInstructions(const QString &t) {
        QSqlQuery q; q.prepare(QStringLiteral("UPDATE users SET instructions=? WHERE id=?")); q.addBindValue(t); q.addBindValue(m_u.id);
        if (!Db::run(q, "talimat")) return QStringLiteral("Kaydedilemedi.");
        m_u.instructions = t; return QString();
    }
    QString changePassword(const QString &oldPw, const QString &n1, const QString &n2) {
        if (!verify(oldPw)) return QStringLiteral("Mevcut parola hatalı.");
        if (n1.size() < 8) return QStringLiteral("Yeni parola en az 8 karakter olmalı.");
        if (n1 != n2) return QStringLiteral("Yeni parolalar eşleşmiyor.");
        const QByteArray salt = randomBytes();
        QSqlQuery q; q.prepare(QStringLiteral("UPDATE users SET salt=?,hash=?,iters=? WHERE id=?"));
        q.addBindValue(salt); q.addBindValue(hashPw(n1, salt, kIters)); q.addBindValue(kIters); q.addBindValue(m_u.id);
        return Db::run(q, "parola") ? QString() : QStringLiteral("Parola değiştirilemedi.");
    }
    QString deleteAccount(const QString &pw) {
        if (!verify(pw)) return QStringLiteral("Parola hatalı.");
        const QString id = m_u.id;
        QSqlQuery q1; q1.prepare(QStringLiteral("DELETE FROM messages WHERE conv_id IN (SELECT id FROM conversations WHERE user_id=?)")); q1.addBindValue(id); Db::run(q1, "hesap sil");
        QSqlQuery q2; q2.prepare(QStringLiteral("DELETE FROM conversations WHERE user_id=?")); q2.addBindValue(id); Db::run(q2, "hesap sil");
        QSqlQuery q3; q3.prepare(QStringLiteral("DELETE FROM users WHERE id=?")); q3.addBindValue(id);
        if (!Db::run(q3, "hesap sil")) return QStringLiteral("Hesap silinemedi.");
        logout(); return QString();
    }
signals:
    void userChanged();
private:
    static QByteArray randomBytes() { quint32 r[4]; QRandomGenerator::system()->fillRange(r); return QByteArray(reinterpret_cast<const char *>(r), int(sizeof(r))); }
    static QByteArray hashPw(const QString &pw, const QByteArray &salt, int it) { return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, pw.toUtf8(), salt, it, 32); }
    static bool ctEq(const QByteArray &a, const QByteArray &b) { if (a.size() != b.size()) return false; int d = 0; for (int i = 0; i < a.size(); ++i) d |= (a[i] ^ b[i]); return d == 0; }
    bool verify(const QString &pw) const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT salt,hash,iters FROM users WHERE id=?")); q.addBindValue(m_u.id);
        if (!q.exec() || !q.next()) return false;
        return ctEq(hashPw(pw, q.value(0).toByteArray(), q.value(2).toInt()), q.value(1).toByteArray());
    }
    void setCurrent(const QString &id) {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT id,first,last,username,email,initials,instructions,created FROM users WHERE id=?")); q.addBindValue(id);
        if (q.exec() && q.next()) {
            m_u.id = q.value(0).toString(); m_u.first = q.value(1).toString(); m_u.last = q.value(2).toString(); m_u.username = q.value(3).toString();
            m_u.email = q.value(4).toString(); m_u.initials = q.value(5).toString(); m_u.instructions = q.value(6).toString(); m_u.created = q.value(7).toLongLong();
            Cfg::set("lastUser", id);
        }
        emit userChanged();
    }
    UserRec m_u; int m_fails = 0; qint64 m_lockUntil = 0;
};
static UserManager *g_um = nullptr;
static UserManager *UM() { return g_um; }

// ---------- sohbetler ----------
struct ConvInfo { QString id, title, project, model; qint64 created = 0, updated = 0; bool pinned = false; };
struct MsgRec { qint64 id = 0; QString conv, role, content, model; qint64 ts = 0; QStringList images; int feedback = 0; };

class ConversationManager : public QObject {
    Q_OBJECT
public:
    explicit ConversationManager(QObject *p = nullptr) : QObject(p) {}
    static QString like(const QString &s) { QString e = s; e.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_"); return "%" + e + "%"; }
    QList<ConvInfo> list(const QString &search = QString(), const QString &project = QString()) const {
        QString sql = QStringLiteral("SELECT id,title,created,updated,pinned,project,model FROM conversations WHERE user_id=?");
        if (!project.isEmpty()) sql += QStringLiteral(" AND project=?");
        if (!search.isEmpty()) sql += QStringLiteral(" AND (title LIKE ? ESCAPE '\\' OR id IN (SELECT conv_id FROM messages WHERE content LIKE ? ESCAPE '\\'))");
        sql += QStringLiteral(" ORDER BY pinned DESC, updated DESC");
        QSqlQuery q; q.prepare(sql); q.addBindValue(UM()->user().id);
        if (!project.isEmpty()) q.addBindValue(project);
        if (!search.isEmpty()) { q.addBindValue(like(search)); q.addBindValue(like(search)); }
        QList<ConvInfo> out; if (!Db::run(q, "sohbet listesi")) return out;
        while (q.next()) out.append(rowToInfo(q));
        return out;
    }
    ConvInfo get(const QString &id) const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT id,title,created,updated,pinned,project,model FROM conversations WHERE id=? AND user_id=?")); q.addBindValue(id); q.addBindValue(UM()->user().id);
        if (q.exec() && q.next()) return rowToInfo(q);
        return ConvInfo();
    }
    QString lastId() const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT id FROM conversations WHERE user_id=? ORDER BY updated DESC LIMIT 1")); q.addBindValue(UM()->user().id);
        return (q.exec() && q.next()) ? q.value(0).toString() : QString();
    }
    int count(const QString &project) const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT COUNT(*) FROM conversations WHERE user_id=? AND project=?")); q.addBindValue(UM()->user().id); q.addBindValue(project);
        return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
    }
    QString create(const QString &title = QStringLiteral("Yeni sohbet"), const QString &model = QString()) {
        const QString id = newId(); const qint64 t = nowMs();
        QSqlQuery q; q.prepare(QStringLiteral("INSERT INTO conversations(id,user_id,title,created,updated,pinned,project,model) VALUES(?,?,?,?,?,0,'',?)"));
        q.addBindValue(id); q.addBindValue(UM()->user().id); q.addBindValue(title); q.addBindValue(t); q.addBindValue(t); q.addBindValue(model);
        if (!Db::run(q, "sohbet oluştur")) return QString();
        emit changed(); return id;
    }
    void rename(const QString &id, const QString &t) { simple("UPDATE conversations SET title=? WHERE id=?", {t.trimmed().isEmpty() ? QStringLiteral("Yeni sohbet") : t.trimmed(), id}); }
    void setPinned(const QString &id, bool p) { simple("UPDATE conversations SET pinned=? WHERE id=?", {p ? 1 : 0, id}); }
    void setProject(const QString &id, const QString &pr) { simple("UPDATE conversations SET project=? WHERE id=?", {pr, id}); }
    void touch(const QString &id) { simple("UPDATE conversations SET updated=? WHERE id=?", {nowMs(), id}); }
    void remove(const QString &id) {
        QSqlQuery a; a.prepare(QStringLiteral("DELETE FROM messages WHERE conv_id=?")); a.addBindValue(id); Db::run(a, "sohbet sil");
        simple("DELETE FROM conversations WHERE id=? AND user_id=?", {id, UM()->user().id});
    }
    void renameProject(const QString &o, const QString &n) { simple("UPDATE conversations SET project=? WHERE project=? AND user_id=?", {n, o, UM()->user().id}); }
    void deleteAll() {
        QSqlQuery a; a.prepare(QStringLiteral("DELETE FROM messages WHERE conv_id IN (SELECT id FROM conversations WHERE user_id=?)")); a.addBindValue(UM()->user().id); Db::run(a, "tümünü sil");
        simple("DELETE FROM conversations WHERE user_id=?", {UM()->user().id});
    }
    QList<MsgRec> messages(const QString &conv) const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT id,conv_id,role,content,ts,images,model,feedback FROM messages WHERE conv_id=? ORDER BY id")); q.addBindValue(conv);
        QList<MsgRec> out; if (!Db::run(q, "mesajlar")) return out;
        while (q.next()) {
            MsgRec m; m.id = q.value(0).toLongLong(); m.conv = q.value(1).toString(); m.role = q.value(2).toString(); m.content = q.value(3).toString();
            m.ts = q.value(4).toLongLong(); m.model = q.value(6).toString(); m.feedback = q.value(7).toInt();
            for (const QJsonValue &v : QJsonDocument::fromJson(q.value(5).toString().toUtf8()).array()) m.images << v.toString();
            out.append(m);
        }
        return out;
    }
    qint64 addMessage(const MsgRec &m) {
        QJsonArray arr; for (const QString &s : m.images) arr.append(s);
        QSqlQuery q; q.prepare(QStringLiteral("INSERT INTO messages(conv_id,role,content,ts,images,model,feedback) VALUES(?,?,?,?,?,?,0)"));
        q.addBindValue(m.conv); q.addBindValue(m.role); q.addBindValue(m.content); q.addBindValue(m.ts ? m.ts : nowMs());
        q.addBindValue(QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact))); q.addBindValue(m.model);
        if (!Db::run(q, "mesaj kaydet")) return 0;
        const qint64 id = q.lastInsertId().toLongLong();
        touch(m.conv); return id;
    }
    void setFeedback(qint64 id, int f) { QSqlQuery q; q.prepare(QStringLiteral("UPDATE messages SET feedback=? WHERE id=?")); q.addBindValue(f); q.addBindValue(id); Db::run(q, "geri bildirim"); }
    void deleteMessage(qint64 id) { QSqlQuery q; q.prepare(QStringLiteral("DELETE FROM messages WHERE id=?")); q.addBindValue(id); Db::run(q, "mesaj sil"); }
    QList<QPair<QString, QString>> allImages() const {
        QSqlQuery q; q.prepare(QStringLiteral("SELECT images,conv_id FROM messages WHERE role='user' AND images<>'[]' AND conv_id IN (SELECT id FROM conversations WHERE user_id=?) ORDER BY id DESC")); q.addBindValue(UM()->user().id);
        QList<QPair<QString, QString>> out; if (!Db::run(q, "görseller")) return out;
        while (q.next()) for (const QJsonValue &v : QJsonDocument::fromJson(q.value(0).toString().toUtf8()).array()) out.append({v.toString(), q.value(1).toString()});
        return out;
    }
    QJsonArray exportAll() const {
        QJsonArray all;
        for (const ConvInfo &c : list()) {
            QJsonArray ms;
            for (const MsgRec &m : messages(c.id)) ms.append(QJsonObject{{"role", m.role}, {"content", m.content}, {"time", QDateTime::fromMSecsSinceEpoch(m.ts).toString(Qt::ISODate)}, {"model", m.model}});
            all.append(QJsonObject{{"id", c.id}, {"title", c.title}, {"project", c.project}, {"pinned", c.pinned}, {"updated", QDateTime::fromMSecsSinceEpoch(c.updated).toString(Qt::ISODate)}, {"messages", ms}});
        }
        return all;
    }
signals:
    void changed();
private:
    static ConvInfo rowToInfo(const QSqlQuery &q) {
        ConvInfo c; c.id = q.value(0).toString(); c.title = q.value(1).toString(); c.created = q.value(2).toLongLong(); c.updated = q.value(3).toLongLong();
        c.pinned = q.value(4).toInt() != 0; c.project = q.value(5).toString(); c.model = q.value(6).toString(); return c;
    }
    void simple(const char *sql, const QVariantList &vals) {
        QSqlQuery q; q.prepare(QString::fromUtf8(sql)); for (const QVariant &v : vals) q.addBindValue(v);
        if (Db::run(q, "güncelle")) emit changed();
    }
};
static ConversationManager *g_cm = nullptr;
static ConversationManager *CM() { return g_cm; }

// ============================================================================
// 3) Sağlayıcılar, ağ katmanı, yerel sunucu
// ============================================================================
enum class PK { OpenAI = 0, Anthropic, DeepSeek, Gemini, Compat, Custom, Local };
static QString pkName(PK k) {
    switch (k) {
    case PK::OpenAI: return QStringLiteral("OpenAI");
    case PK::Anthropic: return QStringLiteral("Anthropic");
    case PK::DeepSeek: return QStringLiteral("DeepSeek");
    case PK::Gemini: return QStringLiteral("Google Gemini");
    case PK::Compat: return QStringLiteral("OpenAI uyumlu API");
    case PK::Custom: return QStringLiteral("Özel API uç noktası");
    case PK::Local: return QStringLiteral("RÜZGAR Local Server");
    }
    return QString();
}
static QString pkDefaultUrl(PK k) {
    switch (k) {
    case PK::OpenAI: return QStringLiteral("https://api.openai.com/v1");
    case PK::Anthropic: return QStringLiteral("https://api.anthropic.com/v1");
    case PK::DeepSeek: return QStringLiteral("https://api.deepseek.com/v1");
    case PK::Gemini: return QStringLiteral("https://generativelanguage.googleapis.com/v1beta");
    case PK::Local: return QStringLiteral("http://127.0.0.1:8765/v1");
    default: return QString();
    }
}
static QString pkDefaultModel(PK k) {
    switch (k) {
    case PK::OpenAI: return QStringLiteral("gpt-4o");
    case PK::Anthropic: return QStringLiteral("claude-sonnet-5-5");
    case PK::DeepSeek: return QStringLiteral("deepseek-chat");
    case PK::Gemini: return QStringLiteral("gemini-2.5-flash");
    default: return QString();
    }
}
static QString trimUrl(QString u) { u = u.trimmed(); while (u.endsWith('/')) u.chop(1); return u; }
static QString validateBaseUrl(const QString &u) {
    const QUrl url(trimUrl(u));
    if (u.trimmed().isEmpty()) return QStringLiteral("Base URL boş olamaz.");
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")))
        return QStringLiteral("Geçersiz sunucu adresi: http:// veya https:// ile başlayan geçerli bir URL girin.");
    return QString();
}

struct ProviderCfg {
    QString id, name; PK kind = PK::OpenAI; QString baseUrl, model, org; int timeout = 60, vision = -1;
    bool lastOk = false; QString lastMsg; qint64 lastTested = 0;
    QJsonObject toJson() const {
        return QJsonObject{{"id", id}, {"name", name}, {"kind", int(kind)}, {"baseUrl", baseUrl}, {"model", model}, {"org", org}, {"timeout", timeout}, {"vision", vision},
                           {"lastOk", lastOk}, {"lastMsg", lastMsg}, {"lastTested", double(lastTested)}};
    }
    static ProviderCfg fromJson(const QJsonObject &o) {
        ProviderCfg c; c.id = o["id"].toString(); c.name = o["name"].toString(); c.kind = PK(qBound(0, o["kind"].toInt(), 6)); c.baseUrl = o["baseUrl"].toString();
        c.model = o["model"].toString(); c.org = o["org"].toString(); c.timeout = o["timeout"].toInt(60); c.vision = o["vision"].toInt(-1);
        c.lastOk = o["lastOk"].toBool(); c.lastMsg = o["lastMsg"].toString(); c.lastTested = qint64(o["lastTested"].toDouble()); return c;
    }
};
struct ChatMsg { QString role, text; QStringList images; };
struct GenOpts { int thinking = 0; int maxTokens = 4096; QString system; };

static bool thinkingSupported(PK k, const QString &model) {
    const QString m = model.toLower();
    switch (k) {
    case PK::OpenAI: return m.startsWith("o1") || m.startsWith("o3") || m.startsWith("o4") || m.startsWith("gpt-5");
    case PK::Anthropic: return m.contains("claude");
    case PK::Gemini: return m.contains("gemini-2.5") || m.contains("gemini-3");
    default: return false;
    }
}
static int visionHeuristic(PK k, const QString &model) {
    const QString m = model.toLower();
    switch (k) {
    case PK::OpenAI: return (m.startsWith("gpt-4o") || m.startsWith("gpt-4.1") || m.startsWith("gpt-4.5") || m.startsWith("gpt-4-turbo") || m.startsWith("gpt-5") || m.startsWith("o1") || m.startsWith("o3") || m.startsWith("o4")) ? 1 : -1;
    case PK::Anthropic: return m.contains("claude") ? 1 : -1;
    case PK::Gemini: return m.contains("gemini") ? 1 : -1;
    case PK::DeepSeek: return 0;
    default: return -1;
    }
}

static bool encodeImage(const QString &path, QString *mime, QString *b64, QString *err) {
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isReadable()) { *err = QStringLiteral("Görsel dosyası bulunamadı veya okunamıyor: ") + fi.fileName(); return false; }
    if (fi.size() > 20LL * 1024 * 1024) { *err = QStringLiteral("Görsel çok büyük (en fazla 20 MB): ") + fi.fileName(); return false; }
    const QString ext = fi.suffix().toLower(); QByteArray data;
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp") {
        QFile f(path); if (!f.open(QIODevice::ReadOnly)) { *err = QStringLiteral("Görsel açılamadı: ") + fi.fileName(); return false; }
        data = f.readAll(); *mime = ext == "png" ? "image/png" : (ext == "webp" ? "image/webp" : "image/jpeg");
    } else {
        QImage img(path); if (img.isNull()) { *err = QStringLiteral("Görsel biçimi okunamadı: ") + fi.fileName(); return false; }
        QBuffer buf(&data); buf.open(QIODevice::WriteOnly);
        if (!img.save(&buf, "PNG")) { *err = QStringLiteral("Görsel dönüştürülemedi: ") + fi.fileName(); return false; }
        *mime = "image/png";
    }
    *b64 = QString::fromLatin1(data.toBase64()); return true;
}
static QString serverMessage(const QByteArray &raw) {
    if (raw.trimmed().isEmpty()) return QString();
    const QJsonDocument d = QJsonDocument::fromJson(raw);
    QJsonObject o = d.isArray() ? d.array().first().toObject() : d.object();
    const QJsonValue e = o.value("error");
    if (e.isObject()) return e.toObject().value("message").toString();
    if (e.isString()) return e.toString();
    if (o.contains("message")) return o.value("message").toString();
    if (o.contains("detail")) return o.value("detail").toString();
    if (d.isNull()) return QString::fromUtf8(raw.left(200)).simplified();
    return QString();
}
static QString apiErrorText(int http, QNetworkReply::NetworkError ne, const QString &srvIn, const ProviderCfg &c, const QString &key, const QString &es) {
    QString srv = srvIn; if (!key.isEmpty()) srv.replace(key, QStringLiteral("***"));
    const QUrl u(c.baseUrl); const QString hostp = u.host() + (u.port() > 0 ? ":" + QString::number(u.port()) : QString());
    QString base;
    if (http >= 400) {
        if (http == 401 || http == 403) base = QStringLiteral("Geçersiz API anahtarı veya yetkisiz erişim (HTTP %1). Anahtarı Ayarlar → API ve Bağlantılar bölümünden kontrol edin.").arg(http);
        else if (http == 404) base = QStringLiteral("Uç nokta veya model bulunamadı (HTTP 404). Base URL ve model kimliğini kontrol edin.");
        else if (http == 400 || http == 422) base = QStringLiteral("İstek reddedildi (HTTP %1): model desteklenmiyor veya istek biçimi geçersiz olabilir.").arg(http);
        else if (http == 408) base = QStringLiteral("Sunucu isteği zaman aşımına uğrattı (HTTP 408).");
        else if (http == 413) base = QStringLiteral("İstek çok büyük (HTTP 413). Görsel veya mesaj boyutunu azaltın.");
        else if (http == 429) base = QStringLiteral("İstek sınırı veya kota aşıldı (HTTP 429).");
        else if (http >= 500) base = QStringLiteral("Sağlayıcı sunucu hatası (HTTP %1). Daha sonra tekrar deneyin.").arg(http);
        else base = QStringLiteral("HTTP hatası %1.").arg(http);
    } else {
        switch (ne) {
        case QNetworkReply::HostNotFoundError: base = QStringLiteral("Sunucu adı çözümlenemedi (%1). Adresi ve ağ bağlantınızı kontrol edin.").arg(hostp); break;
        case QNetworkReply::ConnectionRefusedError: base = QStringLiteral("Bağlantı reddedildi (%1). Sunucu çalışıyor mu?").arg(hostp); break;
        case QNetworkReply::TimeoutError: base = QStringLiteral("Bağlantı zaman aşımına uğradı (%1 sn).").arg(c.timeout); break;
        case QNetworkReply::RemoteHostClosedError: base = QStringLiteral("Sunucu bağlantıyı beklenmedik şekilde kapattı."); break;
        case QNetworkReply::SslHandshakeFailedError: base = QStringLiteral("TLS/SSL bağlantısı kurulamadı: ") + es; break;
        case QNetworkReply::NetworkSessionFailedError: case QNetworkReply::TemporaryNetworkFailureError: case QNetworkReply::UnknownNetworkError:
            base = QStringLiteral("Ağ kullanılamıyor. İnternet bağlantınızı kontrol edin. (%1)").arg(es); break;
        default: base = es.isEmpty() ? QStringLiteral("Bilinmeyen ağ hatası.") : es; break;
        }
    }
    if (!srv.isEmpty()) base += QStringLiteral("\nSunucu mesajı: ") + srv.left(400);
    return base;
}

class ApiJob : public QObject {
    Q_OBJECT
public:
    ApiJob(ProviderCfg cfg, QString key, QString model, QList<ChatMsg> msgs, GenOpts o, QObject *parent)
        : QObject(parent), m_cfg(std::move(cfg)), m_key(std::move(key)), m_model(std::move(model)), m_msgs(normalize(std::move(msgs))), m_o(std::move(o)) {}
    void start() {
        const QString v = validateBaseUrl(m_cfg.baseUrl);
        if (!v.isEmpty()) { fail(v); return; }
        QNetworkRequest req; QByteArray body; QString err;
        if (!build(req, body, &err)) { fail(err); return; }
        req.setTransferTimeout(qMax(5, m_cfg.timeout) * 1000);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        m_reply = nam()->post(req, body);
        connect(m_reply, &QNetworkReply::readyRead, this, &ApiJob::onReady);
        connect(m_reply, &QNetworkReply::finished, this, &ApiJob::onFinished);
    }
    void abort() { m_aborted = true; if (m_reply) m_reply->abort(); }
signals:
    void delta(const QString &t);
    void finished();
    void failed(const QString &msg);
private:
    static QList<ChatMsg> normalize(QList<ChatMsg> in) {
        QList<ChatMsg> out;
        for (ChatMsg &m : in) {
            if (m.text.trimmed().isEmpty() && m.images.isEmpty()) continue;
            if (!out.isEmpty() && out.last().role == m.role) { out.last().text += "\n\n" + m.text; out.last().images += m.images; }
            else out.append(m);
        }
        return out;
    }
    void fail(const QString &msg) { QTimer::singleShot(0, this, [this, msg] { emit failed(msg); }); }
    bool build(QNetworkRequest &req, QByteArray &body, QString *err) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "text/event-stream");
        const QString base = trimUrl(m_cfg.baseUrl);
        QJsonObject o;
        if (m_cfg.kind == PK::Anthropic) {
            req.setUrl(QUrl(base + "/messages"));
            req.setRawHeader("x-api-key", m_key.toUtf8()); req.setRawHeader("anthropic-version", "2023-06-01");
            QJsonArray arr;
            for (const ChatMsg &m : m_msgs) {
                QJsonArray parts;
                for (const QString &p : m.images) {
                    QString mime, b64; if (!encodeImage(p, &mime, &b64, err)) return false;
                    parts.append(QJsonObject{{"type", "image"}, {"source", QJsonObject{{"type", "base64"}, {"media_type", mime}, {"data", b64}}}});
                }
                if (!m.text.isEmpty()) parts.append(QJsonObject{{"type", "text"}, {"text", m.text}});
                arr.append(QJsonObject{{"role", m.role}, {"content", parts}});
            }
            int maxTok = qMax(256, m_o.maxTokens);
            o = QJsonObject{{"model", m_model}, {"messages", arr}, {"stream", true}};
            if (!m_o.system.isEmpty()) o["system"] = m_o.system;
            if (m_o.thinking > 0 && thinkingSupported(m_cfg.kind, m_model)) {
                static const int budgets[4] = {0, 2048, 8192, 16000};
                const int bud = budgets[qBound(0, m_o.thinking, 3)];
                o["thinking"] = QJsonObject{{"type", "enabled"}, {"budget_tokens", bud}};
                maxTok = qMax(maxTok, bud + 4096);
            }
            o["max_tokens"] = maxTok;
        } else if (m_cfg.kind == PK::Gemini) {
            QString mdl = m_model; if (mdl.startsWith("models/")) mdl = mdl.mid(7);
            req.setUrl(QUrl(base + "/models/" + mdl + ":streamGenerateContent?alt=sse"));
            req.setRawHeader("x-goog-api-key", m_key.toUtf8());
            QJsonArray contents;
            for (const ChatMsg &m : m_msgs) {
                QJsonArray parts;
                if (!m.text.isEmpty()) parts.append(QJsonObject{{"text", m.text}});
                for (const QString &p : m.images) {
                    QString mime, b64; if (!encodeImage(p, &mime, &b64, err)) return false;
                    parts.append(QJsonObject{{"inline_data", QJsonObject{{"mime_type", mime}, {"data", b64}}}});
                }
                contents.append(QJsonObject{{"role", m.role == "assistant" ? "model" : "user"}, {"parts", parts}});
            }
            o["contents"] = contents;
            if (!m_o.system.isEmpty()) o["systemInstruction"] = QJsonObject{{"parts", QJsonArray{QJsonObject{{"text", m_o.system}}}}};
            if (m_o.thinking > 0 && thinkingSupported(m_cfg.kind, m_model)) {
                static const int budgets[4] = {0, 1024, 8192, 24576};
                o["generationConfig"] = QJsonObject{{"thinkingConfig", QJsonObject{{"thinkingBudget", budgets[qBound(0, m_o.thinking, 3)]}}}};
            }
        } else {  // OpenAI, DeepSeek, uyumlu, özel, yerel
            QString url = base; if (!url.endsWith("/chat/completions")) url += "/chat/completions";
            req.setUrl(QUrl(url));
            if (!m_key.isEmpty()) req.setRawHeader("Authorization", ("Bearer " + m_key).toUtf8());
            if (!m_cfg.org.isEmpty() && (m_cfg.kind == PK::OpenAI || m_cfg.kind == PK::Compat)) req.setRawHeader("OpenAI-Organization", m_cfg.org.toUtf8());
            QJsonArray arr;
            if (!m_o.system.isEmpty()) arr.append(QJsonObject{{"role", "system"}, {"content", m_o.system}});
            for (const ChatMsg &m : m_msgs) {
                if (m.images.isEmpty()) { arr.append(QJsonObject{{"role", m.role}, {"content", m.text}}); continue; }
                QJsonArray parts;
                if (!m.text.isEmpty()) parts.append(QJsonObject{{"type", "text"}, {"text", m.text}});
                for (const QString &p : m.images) {
                    QString mime, b64; if (!encodeImage(p, &mime, &b64, err)) return false;
                    parts.append(QJsonObject{{"type", "image_url"}, {"image_url", QJsonObject{{"url", "data:" + mime + ";base64," + b64}}}});
                }
                arr.append(QJsonObject{{"role", m.role}, {"content", parts}});
            }
            o = QJsonObject{{"model", m_model}, {"messages", arr}, {"stream", true}};
            if (m_cfg.kind == PK::OpenAI && thinkingSupported(m_cfg.kind, m_model)) {
                static const char *eff[4] = {"low", "medium", "high", "high"};
                o["reasoning_effort"] = QString::fromLatin1(eff[qBound(0, m_o.thinking, 3)]);
            }
        }
        body = QJsonDocument(o).toJson(QJsonDocument::Compact);
        return true;
    }
    void emitDelta(const QString &t) { if (t.isEmpty()) return; m_got = true; emit delta(t); }
    void handleObj(const QJsonObject &o) {
        if (o.contains("error") && !o.value("error").isNull()) { const QString e = serverMessage(QJsonDocument(o).toJson()); if (m_err.isEmpty()) m_err = e.isEmpty() ? QStringLiteral("Sağlayıcı bir hata bildirdi.") : e; return; }
        if (m_cfg.kind == PK::Anthropic) {
            const QString t = o.value("type").toString();
            if (t == "content_block_delta") { const QJsonObject d = o.value("delta").toObject(); if (d.value("type").toString() == "text_delta") emitDelta(d.value("text").toString()); }
            else if (t == "message_stop") m_done = true;
            else if (t == "message") { for (const QJsonValue &v : o.value("content").toArray()) emitDelta(v.toObject().value("text").toString()); }
        } else if (m_cfg.kind == PK::Gemini) {
            const QJsonObject pf = o.value("promptFeedback").toObject();
            if (pf.contains("blockReason") && m_err.isEmpty()) m_err = QStringLiteral("İstek sağlayıcı tarafından engellendi (%1).").arg(pf.value("blockReason").toString());
            for (const QJsonValue &cv : o.value("candidates").toArray()) {
                const QJsonObject c = cv.toObject();
                for (const QJsonValue &pv : c.value("content").toObject().value("parts").toArray()) { const QJsonObject p = pv.toObject(); if (!p.value("thought").toBool()) emitDelta(p.value("text").toString()); }
                const QString fr = c.value("finishReason").toString();
                if ((fr == "SAFETY" || fr == "RECITATION" || fr == "BLOCKLIST" || fr == "PROHIBITED_CONTENT") && !m_got && m_err.isEmpty()) m_err = QStringLiteral("Yanıt güvenlik filtresi nedeniyle engellendi (%1).").arg(fr);
            }
        } else {
            for (const QJsonValue &cv : o.value("choices").toArray()) {
                const QJsonObject c = cv.toObject();
                const QJsonValue dc = c.value("delta").toObject().value("content");
                if (dc.isString()) emitDelta(dc.toString());
                else if (c.value("message").isObject()) emitDelta(c.value("message").toObject().value("content").toString());
                else if (c.value("text").isString()) emitDelta(c.value("text").toString());
            }
        }
    }
    void parseLine(const QByteArray &line) {
        if (!line.startsWith("data:")) return;
        const QByteArray p = line.mid(5).trimmed();
        if (p.isEmpty()) return;
        if (p == "[DONE]") { m_done = true; return; }
        const QJsonDocument d = QJsonDocument::fromJson(p);
        if (d.isObject()) handleObj(d.object());
    }
    void feed(const QByteArray &chunk) {
        m_buf += chunk; int nl;
        while ((nl = m_buf.indexOf('\n')) >= 0) { const QByteArray line = m_buf.left(nl).trimmed(); m_buf.remove(0, nl + 1); parseLine(line); }
    }
    void onReady() {
        const QByteArray chunk = m_reply->readAll();
        if (!m_known) {
            m_status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (m_status > 0) { m_known = true; m_sse = m_reply->header(QNetworkRequest::ContentTypeHeader).toString().contains("event-stream"); }
        }
        if (m_status >= 400 || !m_sse) { if (m_raw.size() < (2 << 20)) m_raw += chunk; return; }
        feed(chunk);
    }
    void onFinished() {
        onReady();
        const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto ne = m_reply->error(); const QString es = m_reply->errorString();
        m_reply->deleteLater(); m_reply = nullptr;
        if (m_aborted) { emit finished(); return; }
        if (ne != QNetworkReply::NoError || status >= 400) { emit failed(apiErrorText(status, ne, serverMessage(m_raw), m_cfg, m_key, es)); return; }
        if (!m_sse && !m_raw.isEmpty()) {
            if (m_raw.startsWith("data:") || m_raw.contains("\ndata:")) feed(m_raw + "\n");
            else {
                const QJsonDocument d = QJsonDocument::fromJson(m_raw);
                if (d.isArray()) { for (const QJsonValue &v : d.array()) handleObj(v.toObject()); }
                else if (d.isObject()) handleObj(d.object());
                else { emit failed(QStringLiteral("Sunucu beklenen biçimde yanıt vermedi (JSON/SSE değil). Base URL'yi kontrol edin.")); return; }
            }
        }
        if (!m_buf.trimmed().isEmpty()) feed("\n");
        if (!m_err.isEmpty()) { emit failed(m_err); return; }
        if (!m_got) { emit failed(QStringLiteral("Sağlayıcı boş bir yanıt döndürdü.")); return; }
        emit finished();
    }
    ProviderCfg m_cfg; QString m_key, m_model; QList<ChatMsg> m_msgs; GenOpts m_o;
    QNetworkReply *m_reply = nullptr; QByteArray m_buf, m_raw; QString m_err;
    bool m_aborted = false, m_known = false, m_sse = false, m_got = false, m_done = false; int m_status = 0;
};

// ---------- yerel sunucu bağlantısı (bulut yapılandırmalarından ayrı) ----------
class LocalServerConnection : public QObject {
    Q_OBJECT
public:
    explicit LocalServerConnection(QObject *p = nullptr) : QObject(p) { m_models = Cfg::get("localModels").toStringList(); }
    QString address() const { return Cfg::get("localAddr", "http://127.0.0.1").toString(); }
    int port() const { return Cfg::get("localPort", 8765).toInt(); }
    QString model() const { return Cfg::get("localModel").toString(); }
    QString key() const { if (!m_keyLoaded) { m_key = SecretStore::load("local"); m_keyLoaded = true; } return m_key; }
    QStringList models() const { return m_models; }
    int state() const { return m_state; }   // 0 test edilmedi, 1 bağlı, 2 hata
    QString statusText() const { return m_state == 1 ? QStringLiteral("Bağlı") : (m_state == 2 ? QStringLiteral("Bağlı değil") : QStringLiteral("Test edilmedi")); }
    QString lastMessage() const { return m_msg; }
    static QString normalize(QString a) { a = a.trimmed(); if (!a.contains("://")) a = "http://" + a; while (a.endsWith('/')) a.chop(1); return a; }
    static QString validate(const QString &addr, int port) {
        const QUrl u(normalize(addr));
        if (addr.trimmed().isEmpty() || !u.isValid() || u.host().isEmpty() || (u.scheme() != "http" && u.scheme() != "https")) return QStringLiteral("Geçersiz sunucu adresi. Örnek: http://127.0.0.1");
        if (port < 1 || port > 65535) return QStringLiteral("Geçersiz bağlantı noktası (1–65535).");
        return QString();
    }
    QString baseUrl() const { QUrl u(normalize(address())); u.setPort(port()); return trimUrl(u.toString()) + "/v1"; }
    QString save(const QString &addr, int port, const QString &key, const QString &model) {
        const QString v = validate(addr, port); if (!v.isEmpty()) return v;
        Cfg::set("localAddr", normalize(addr)); Cfg::set("localPort", port); Cfg::set("localModel", model.trimmed());
        m_key = key; m_keyLoaded = true;
        if (!SecretStore::save("local", key) && !key.isEmpty()) emit ThemeBus::I().error(QStringLiteral("Sunucu anahtarı Windows Kimlik Bilgisi Yöneticisi'ne kaydedilemedi; yalnızca bu oturumda geçerli."));
        m_state = 0; emit statusChanged(); return QString();
    }
    void setModel(const QString &m) { Cfg::set("localModel", m); emit statusChanged(); }
    ProviderCfg cfg() const {
        ProviderCfg c; c.id = "local"; c.name = pkName(PK::Local); c.kind = PK::Local; c.baseUrl = baseUrl(); c.model = model();
        c.timeout = Cfg::get("localTimeout", 120).toInt(); c.vision = -1; c.lastOk = m_state == 1; return c;
    }
    void test(const QString &addr, int port, const QString &keyIn) {
        const QString v = validate(addr, port);
        if (!v.isEmpty()) { finishTest(false, v, {}); return; }
        QUrl u(normalize(addr)); u.setPort(port);
        QNetworkRequest req(QUrl(trimUrl(u.toString()) + "/v1/models"));
        if (!keyIn.isEmpty()) req.setRawHeader("Authorization", ("Bearer " + keyIn).toUtf8());
        req.setTransferTimeout(8000);
        QNetworkReply *r = nam()->get(req);
        connect(r, &QNetworkReply::finished, this, [this, r, keyIn, port] {
            const int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray body = r->readAll(); const auto ne = r->error(); const QString es = r->errorString(); r->deleteLater();
            ProviderCfg pc; pc.baseUrl = r->url().toString(); pc.timeout = 8; pc.kind = PK::Local;
            if (ne != QNetworkReply::NoError || http >= 400) { finishTest(false, apiErrorText(http, ne, serverMessage(body), pc, keyIn, es), {}); return; }
            const QJsonDocument d = QJsonDocument::fromJson(body);
            if (d.isNull()) { finishTest(false, QStringLiteral("Sunucu yanıt verdi ancak /v1/models JSON biçiminde değil. Bu bir RÜZGAR/OpenAI uyumlu sunucu mu?"), {}); return; }
            QStringList ms;
            for (const QJsonValue &v : d.object().value("data").toArray()) ms << v.toObject().value("id").toString();
            if (ms.isEmpty()) for (const QJsonValue &v : d.object().value("models").toArray()) { const QJsonObject o = v.toObject(); ms << (o.contains("model") ? o.value("model").toString() : o.value("name").toString()); }
            ms.removeAll(QString());
            finishTest(true, ms.isEmpty() ? QStringLiteral("Bağlantı başarılı; sunucu model listesi döndürmedi.") : QStringLiteral("Bağlantı başarılı — %1 model listelendi.").arg(ms.size()), ms);
        });
    }
signals:
    void statusChanged();
    void testDone(bool ok, const QString &msg, const QStringList &models);
private:
    void finishTest(bool ok, const QString &msg, const QStringList &models) {
        m_state = ok ? 1 : 2; m_msg = msg;
        if (ok) { m_models = models; Cfg::set("localModels", models); if (model().isEmpty() && !models.isEmpty()) Cfg::set("localModel", models.first()); }
        emit statusChanged(); emit testDone(ok, msg, models);
    }
    QStringList m_models; mutable QString m_key; mutable bool m_keyLoaded = false; int m_state = 0; QString m_msg;
};
static LocalServerConnection *g_ls = nullptr;
static LocalServerConnection *LS() { return g_ls; }

// ---------- API yöneticisi ----------
class ApiManager : public QObject {
    Q_OBJECT
public:
    explicit ApiManager(QObject *p = nullptr) : QObject(p) {
        for (const QJsonValue &v : QJsonDocument::fromJson(Cfg::get("providers").toString().toUtf8()).array()) m_list << ProviderCfg::fromJson(v.toObject());
        m_default = Cfg::get("defaultProvider").toString();
    }
    QList<ProviderCfg> providers() const { return m_list; }
    ProviderCfg find(const QString &id, bool *ok = nullptr) const { for (const ProviderCfg &c : m_list) if (c.id == id) { if (ok) *ok = true; return c; } if (ok) *ok = false; return ProviderCfg(); }
    QString defaultId() const { return m_default; }
    void setDefault(const QString &id) { m_default = id; persist(); emit changed(); }
    QString key(const QString &id) const {
        auto it = m_keys.constFind(id); if (it != m_keys.constEnd()) return *it;
        const QString k = SecretStore::load("prov:" + id); m_keys.insert(id, k); return k;
    }
    QString keyFor(const ProviderCfg &c) const { return c.kind == PK::Local ? LS()->key() : key(c.id); }
    QString save(ProviderCfg c, const QString &newKey, bool keyChanged) {
        c.name = c.name.trimmed(); if (c.name.isEmpty()) c.name = pkName(c.kind);
        const QString v = validateBaseUrl(c.baseUrl); if (!v.isEmpty()) return v;
        if (c.id.isEmpty()) c.id = newId();
        bool found = false;
        for (ProviderCfg &e : m_list) if (e.id == c.id) { c.lastOk = e.lastOk; c.lastMsg = e.lastMsg; c.lastTested = e.lastTested; e = c; found = true; }
        if (!found) m_list << c;
        if (keyChanged) {
            m_keys.insert(c.id, newKey);
            if (!SecretStore::save("prov:" + c.id, newKey) && !newKey.isEmpty()) emit ThemeBus::I().error(QStringLiteral("API anahtarı Windows Kimlik Bilgisi Yöneticisi'ne kaydedilemedi; yalnızca bu oturumda geçerli."));
        }
        if (m_default.isEmpty()) m_default = c.id;
        persist(); emit changed(); return QString();
    }
    void remove(const QString &id) {
        for (int i = 0; i < m_list.size(); ++i) if (m_list[i].id == id) { m_list.removeAt(i); break; }
        SecretStore::save("prov:" + id, QString()); m_keys.remove(id);
        if (m_default == id) m_default = m_list.isEmpty() ? QString() : m_list.first().id;
        persist(); emit changed();
    }
    void markTested(const QString &id, bool ok, const QString &msg) {
        for (ProviderCfg &e : m_list) if (e.id == id) { e.lastOk = ok; e.lastMsg = msg; e.lastTested = nowMs(); persist(); emit changed(); return; }
    }
    ApiJob *chat(const ProviderCfg &c, const QString &key, const QString &model, const QList<ChatMsg> &msgs, const GenOpts &o) { return new ApiJob(c, key, model, msgs, o, this); }
    void test(const ProviderCfg &cfg, const QString &key) {
        const QString id = cfg.id;
        auto early = [this, id](const QString &m) { QTimer::singleShot(0, this, [this, id, m] { emit testDone(id, false, m, QStringList()); }); };
        const QString v = validateBaseUrl(cfg.baseUrl); if (!v.isEmpty()) { early(v); return; }
        const bool needKey = cfg.kind == PK::OpenAI || cfg.kind == PK::Anthropic || cfg.kind == PK::DeepSeek || cfg.kind == PK::Gemini;
        if (needKey && key.trimmed().isEmpty()) { early(QStringLiteral("API anahtarı girilmemiş.")); return; }
        const QString base = trimUrl(cfg.baseUrl); QNetworkRequest req;
        if (cfg.kind == PK::Gemini) { req.setUrl(QUrl(base + "/models?pageSize=200")); req.setRawHeader("x-goog-api-key", key.toUtf8()); }
        else if (cfg.kind == PK::Anthropic) { req.setUrl(QUrl(base + "/models?limit=100")); req.setRawHeader("x-api-key", key.toUtf8()); req.setRawHeader("anthropic-version", "2023-06-01"); }
        else { QString u = base; if (u.endsWith("/chat/completions")) u.chop(17); req.setUrl(QUrl(u + "/models")); if (!key.isEmpty()) req.setRawHeader("Authorization", ("Bearer " + key).toUtf8()); }
        req.setTransferTimeout(qMax(5, cfg.timeout) * 1000);
        QNetworkReply *r = nam()->get(req);
        connect(r, &QNetworkReply::finished, this, [this, r, cfg, key, id] {
            const int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray body = r->readAll(); const auto ne = r->error(); const QString es = r->errorString(); r->deleteLater();
            if (ne != QNetworkReply::NoError || http >= 400) { const QString m = apiErrorText(http, ne, serverMessage(body), cfg, key, es); markTested(id, false, m); emit testDone(id, false, m, QStringList()); return; }
            const QJsonDocument d = QJsonDocument::fromJson(body);
            if (d.isNull()) { const QString m = QStringLiteral("Sunucu yanıt verdi ancak beklenen JSON biçiminde değil. Base URL yanlış olabilir."); markTested(id, false, m); emit testDone(id, false, m, QStringList()); return; }
            QStringList ms; const QJsonObject o = d.object();
            if (cfg.kind == PK::Gemini) {
                for (const QJsonValue &v : o.value("models").toArray()) {
                    const QJsonObject mo = v.toObject(); bool gen = false;
                    for (const QJsonValue &mv : mo.value("supportedGenerationMethods").toArray()) if (mv.toString() == "generateContent") gen = true;
                    QString n = mo.value("name").toString(); if (n.startsWith("models/")) n = n.mid(7);
                    if (gen && !n.isEmpty()) ms << n;
                }
            } else {
                for (const QJsonValue &v : o.value("data").toArray()) ms << v.toObject().value("id").toString();
                if (ms.isEmpty()) for (const QJsonValue &v : o.value("models").toArray()) { if (v.isString()) ms << v.toString(); else ms << v.toObject().value("id").toString(); }
                ms.removeAll(QString());
            }
            QString m = ms.isEmpty() ? QStringLiteral("Bağlantı başarılı; sağlayıcı model listesi döndürmedi.") : QStringLiteral("Bağlantı başarılı — %1 model listelendi.").arg(ms.size());
            if (!ms.isEmpty() && !cfg.model.isEmpty() && !ms.contains(cfg.model)) m += QStringLiteral("\nUyarı: '%1' model kimliği listede bulunamadı.").arg(cfg.model);
            markTested(id, true, m); emit testDone(id, true, m, ms);
        });
    }
signals:
    void changed();
    void testDone(const QString &id, bool ok, const QString &msg, const QStringList &models);
private:
    void persist() { QJsonArray a; for (const ProviderCfg &p : m_list) a.append(p.toJson()); Cfg::set("providers", QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact))); Cfg::set("defaultProvider", m_default); }
    QList<ProviderCfg> m_list; QString m_default; mutable QHash<QString, QString> m_keys;
};
static ApiManager *g_am = nullptr;
static ApiManager *AM() { return g_am; }

// ============================================================================
// 4) Donanım izleme, bellek tahmini, model kataloğu
// ============================================================================
static const double kGiB = 1073741824.0;
struct GpuInfo { QString name; qint64 totalMB = -1, usedMB = -1, freeMB = -1; int util = -1; };
struct HwSnap {
    QString cpuName; int cores = 0; double cpuUsage = -1; qint64 ramTotal = -1, ramUsed = -1, ramAvail = -1;
    QList<GpuInfo> gpus; bool gpuOk = false; QString gpuMsg = QStringLiteral("Unavailable");
};

class HardwareMonitor : public QObject {
    Q_OBJECT
public:
    explicit HardwareMonitor(QObject *p = nullptr) : QObject(p) {
        m_cands = {QStringLiteral("nvidia-smi"), QStringLiteral("C:/Windows/System32/nvidia-smi.exe"), QStringLiteral("C:/Program Files/NVIDIA Corporation/NVSMI/nvidia-smi.exe")};
        m_snap.cpuName = readCpuName(); m_snap.cores = QThread::idealThreadCount();
        m_timer = new QTimer(this); m_timer->setInterval(2000); connect(m_timer, &QTimer::timeout, this, &HardwareMonitor::refresh);
        m_kill = new QTimer(this); m_kill->setSingleShot(true);
        m_proc = new QProcess(this);
#ifdef Q_OS_WIN
        m_proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) { a->flags |= CREATE_NO_WINDOW; });
#endif
        connect(m_kill, &QTimer::timeout, this, [this] { if (m_proc->state() != QProcess::NotRunning) m_proc->kill(); });
        connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) { onSmiDone(code); });
        connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e == QProcess::FailedToStart) onSmiMissing(); });
        refresh();
    }
    void acquire() { if (m_refs++ == 0) { m_timer->start(); refresh(); } }
    void release() { if (m_refs > 0 && --m_refs == 0) m_timer->stop(); }
    const HwSnap &snap() const { return m_snap; }
    double vramTotalGB() const { if (!m_snap.gpuOk) return -1; double t = 0; for (const GpuInfo &g : m_snap.gpus) if (g.totalMB > 0) t += g.totalMB / 1024.0; return t > 0 ? t : -1; }
    double vramFreeGB() const { if (!m_snap.gpuOk) return -1; double t = 0; bool any = false; for (const GpuInfo &g : m_snap.gpus) if (g.freeMB >= 0) { t += g.freeMB / 1024.0; any = true; } return any ? t : -1; }
    double ramAvailGB() const { return m_snap.ramAvail > 0 ? m_snap.ramAvail / kGiB : -1; }
    double ramTotalGB() const { return m_snap.ramTotal > 0 ? m_snap.ramTotal / kGiB : -1; }
public slots:
    void refresh() {
#ifdef Q_OS_WIN
        MEMORYSTATUSEX ms; ZeroMemory(&ms, sizeof(ms)); ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) { m_snap.ramTotal = qint64(ms.ullTotalPhys); m_snap.ramAvail = qint64(ms.ullAvailPhys); m_snap.ramUsed = m_snap.ramTotal - m_snap.ramAvail; }
        FILETIME idle, kern, user;
        if (GetSystemTimes(&idle, &kern, &user)) {
            auto u64 = [](const FILETIME &f) { return (quint64(f.dwHighDateTime) << 32) | quint64(f.dwLowDateTime); };
            const quint64 i = u64(idle), k = u64(kern), u = u64(user);
            if (m_havePrev) { const quint64 di = i - m_pi, dt = (k - m_pk) + (u - m_pu); if (dt > 0) m_snap.cpuUsage = qBound(0.0, 100.0 * (1.0 - double(di) / double(dt)), 100.0); }
            m_pi = i; m_pk = k; m_pu = u; m_havePrev = true;
        }
#endif
        launchSmi();
        emit updated();
    }
signals:
    void updated();
private:
    static QString readCpuName() {
#ifdef Q_OS_WIN
        QSettings r(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), QSettings::NativeFormat);
        const QString n = r.value(QStringLiteral("ProcessorNameString")).toString().simplified();
        if (!n.isEmpty()) return n;
#endif
        return QStringLiteral("Unavailable");
    }
    void launchSmi() {
        if (m_missing || m_proc->state() != QProcess::NotRunning) return;
        m_proc->start(m_cands.at(m_idx), QStringList{QStringLiteral("--query-gpu=name,memory.total,memory.used,memory.free,utilization.gpu"), QStringLiteral("--format=csv,noheader,nounits")});
        m_kill->start(4000);
    }
    void onSmiMissing() {
        m_kill->stop();
        if (m_idx + 1 < m_cands.size()) { ++m_idx; QTimer::singleShot(0, this, [this] { launchSmi(); }); return; }
        m_missing = true; m_snap.gpuOk = false; m_snap.gpus.clear(); m_snap.gpuMsg = QStringLiteral("Unavailable (nvidia-smi bulunamadı; NVIDIA GPU/sürücü gerekli)");
        emit updated();
    }
    void onSmiDone(int code) {
        m_kill->stop();
        const QString out = QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed();
        if (code != 0 || out.isEmpty()) { m_snap.gpuOk = false; m_snap.gpus.clear(); m_snap.gpuMsg = QStringLiteral("Unavailable (nvidia-smi çalıştı ancak veri döndürmedi)"); emit updated(); return; }
        QList<GpuInfo> gl;
        for (const QString &ln : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList f = ln.split(','); if (f.size() < 4) continue;
            auto num = [](const QString &s) { bool ok = false; const qint64 v = s.trimmed().toLongLong(&ok); return ok ? v : qint64(-1); };
            GpuInfo g; g.name = f[0].trimmed(); g.totalMB = num(f[1]); g.usedMB = num(f[2]); g.freeMB = num(f[3]); g.util = f.size() > 4 ? int(num(f[4])) : -1;
            gl << g;
        }
        m_snap.gpus = gl; m_snap.gpuOk = !gl.isEmpty(); m_snap.gpuMsg = m_snap.gpuOk ? QString() : QStringLiteral("Unavailable");
        emit updated();
    }
    HwSnap m_snap; QTimer *m_timer, *m_kill; QProcess *m_proc; QStringList m_cands; int m_idx = 0, m_refs = 0; bool m_missing = false, m_havePrev = false; quint64 m_pi = 0, m_pk = 0, m_pu = 0;
};
static HardwareMonitor *g_hw = nullptr;
static HardwareMonitor *HW() { return g_hw; }

// ---------- model girdisi ve bellek tahmini ----------
struct ModelEntry {
    QString id, name, family; PK kind = PK::Local; QString provId, apiId, quant;
    double paramsB = -1; int ctx = 0, layers = 0, kvHeads = 0, headDim = 0; double vramOverride = -1, ramOverride = -1;
    bool builtin = false, demo = false; int vision = -1;
    QJsonObject toJson() const {
        return QJsonObject{{"id", id}, {"name", name}, {"family", family}, {"kind", int(kind)}, {"provId", provId}, {"apiId", apiId}, {"quant", quant}, {"paramsB", paramsB},
                           {"ctx", ctx}, {"layers", layers}, {"kvHeads", kvHeads}, {"headDim", headDim}, {"vision", vision}};
    }
    static ModelEntry fromJson(const QJsonObject &o) {
        ModelEntry e; e.id = o["id"].toString(); e.name = o["name"].toString(); e.family = o["family"].toString(); e.kind = PK(qBound(0, o["kind"].toInt(), 6));
        e.provId = o["provId"].toString(); e.apiId = o["apiId"].toString(); e.quant = o["quant"].toString(); e.paramsB = o["paramsB"].toDouble(-1);
        e.ctx = o["ctx"].toInt(); e.layers = o["layers"].toInt(); e.kvHeads = o["kvHeads"].toInt(); e.headDim = o["headDim"].toInt(); e.vision = o["vision"].toInt(-1); return e;
    }
    QString location() const { return kind == PK::Local ? QStringLiteral("Yerel") : ((kind == PK::Compat || kind == PK::Custom) ? QStringLiteral("API") : QStringLiteral("Bulut")); }
};
struct MemEst { bool known = false, demo = false; double weights = -1, kv = -1, overhead = 1.0, vram = -1, ram = -1; };
static double bitsPerWeight(const QString &q) {
    const QString s = q.trimmed().toUpper();
    if (s == "F32") return 32; if (s == "F16" || s == "BF16" || s == "FP16") return 16; if (s == "Q8_0" || s == "Q8") return 8.5; if (s == "FP8" || s == "INT8") return 8;
    if (s == "Q6_K" || s == "Q6") return 6.6; if (s == "Q5_K_M" || s == "Q5") return 5.7; if (s == "Q4_K_M") return 4.85; if (s == "Q4" || s == "Q4_0") return 4.5;
    if (s == "INT4" || s == "FP4") return 4.0; if (s == "Q3_K_M" || s == "Q3") return 3.9;
    return -1;
}
static MemEst estimateMem(const ModelEntry &e) {
    MemEst m;
    if (e.vramOverride > 0 || e.ramOverride > 0) { m.known = true; m.demo = true; m.vram = e.vramOverride; m.ram = e.ramOverride; return m; }
    const double b = bitsPerWeight(e.quant);
    if (e.paramsB <= 0 || b <= 0) return m;
    m.weights = e.paramsB * 1e9 * b / 8.0 / kGiB;
    if (e.ctx > 0 && e.layers > 0 && e.kvHeads > 0 && e.headDim > 0) m.kv = 2.0 * e.layers * e.kvHeads * e.headDim * double(e.ctx) * 2.0 / kGiB;  // fp16 KV
    const double tot = m.weights + (m.kv > 0 ? m.kv : 0.0) + m.overhead;
    m.known = true; m.vram = tot; m.ram = tot; return m;
}
enum class Fit { Unknown, Cloud, Api, FitsVram, FitsOffload, FitsRam, Insufficient };
static Fit fitOf(const ModelEntry &e) {
    if (e.kind != PK::Local) return (e.kind == PK::Compat || e.kind == PK::Custom) ? Fit::Api : Fit::Cloud;
    const MemEst m = estimateMem(e); if (!m.known) return Fit::Unknown;
    const double vf = HW()->vramFreeGB(), rf = HW()->ramAvailGB();
    if (vf >= 0) {
        if (m.vram > 0 && m.vram <= vf) return Fit::FitsVram;
        if (rf >= 0) return (m.vram <= vf + rf) ? Fit::FitsOffload : Fit::Insufficient;
        return Fit::Unknown;
    }
    if (rf >= 0 && m.ram > 0) return m.ram <= rf ? Fit::FitsRam : Fit::Insufficient;
    return Fit::Unknown;
}
static QString fitText(Fit f) {
    switch (f) {
    case Fit::Cloud: return QStringLiteral("Bulut modeli");
    case Fit::Api: return QStringLiteral("API modeli");
    case Fit::FitsVram: return QStringLiteral("VRAM'e sığar");
    case Fit::FitsOffload: return QStringLiteral("RAM'e boşaltma ile sığar");
    case Fit::FitsRam: return QStringLiteral("Yalnızca RAM (CPU) ile sığar");
    case Fit::Insufficient: return QStringLiteral("Yetersiz bellek");
    default: return QStringLiteral("Bilinmiyor");
    }
}
static QColor fitColor(Fit f) {
    switch (f) { case Fit::FitsVram: return g_t.ok; case Fit::FitsOffload: case Fit::FitsRam: return g_t.accent; case Fit::Insufficient: return g_t.danger; default: return g_t.text2; }
}

class ModelCatalog : public QObject {
    Q_OBJECT
public:
    explicit ModelCatalog(QObject *p = nullptr) : QObject(p) {
        for (const QJsonValue &v : QJsonDocument::fromJson(Cfg::get("customModels").toString().toUtf8()).array()) { ModelEntry e = ModelEntry::fromJson(v.toObject()); if (!e.id.isEmpty()) m_custom << e; }
        connect(AM(), &ApiManager::changed, this, [this] { emit changed(); });
        connect(LS(), &LocalServerConnection::statusChanged, this, [this] { emit changed(); });
    }
    static QList<ModelEntry> builtins() {
        auto mk = [](const char *id, const QString &name, const QString &fam, PK k, const QString &prov, double pb, const QString &q, int ctx) {
            ModelEntry e; e.id = QString::fromLatin1(id); e.name = name; e.family = fam; e.kind = k; e.provId = prov; e.paramsB = pb; e.quant = q; e.ctx = ctx; e.builtin = true; return e;
        };
        QList<ModelEntry> l;
        l << mk("b.local", QStringLiteral("RÜZGAR Local"), QStringLiteral("RÜZGAR Local Server"), PK::Local, QStringLiteral("local"), -1, QString(), 0);
        ModelEntry ds = mk("b.deepseek", QStringLiteral("DeepSeek"), QStringLiteral("DeepSeek API"), PK::DeepSeek, QString(), -1, QString(), 0); ds.apiId = QStringLiteral("deepseek-chat"); l << ds;
        l << mk("b.qwen", QStringLiteral("Qwen"), QStringLiteral("Örnek profil · yerel"), PK::Local, QStringLiteral("local"), 7, QStringLiteral("Q4_K_M"), 8192);
        l << mk("b.llama", QStringLiteral("Llama"), QStringLiteral("Örnek profil · yerel"), PK::Local, QStringLiteral("local"), 8, QStringLiteral("Q4_K_M"), 8192);
        l << mk("b.mistral", QStringLiteral("Mistral"), QStringLiteral("Örnek profil · yerel"), PK::Local, QStringLiteral("local"), 7, QStringLiteral("Q4_K_M"), 8192);
        ModelEntry d = mk("b.demo70", QStringLiteral("Örnek 70B Q4"), QStringLiteral("Gösterim değerleri"), PK::Local, QStringLiteral("local"), 70, QStringLiteral("Q4"), 8192);
        d.vramOverride = 48; d.ramOverride = 32; d.demo = true; l << d;
        l << mk("b.custom", QStringLiteral("Custom Model"), QStringLiteral("Özel uç nokta"), PK::Custom, QString(), -1, QString(), 0);
        return l;
    }
    QList<ModelEntry> all() const {
        QList<ModelEntry> out; bool hasDs = false, hasCustom = false; const QList<ProviderCfg> provs = AM()->providers();
        for (const ProviderCfg &p : provs) { if (p.kind == PK::DeepSeek) hasDs = true; if (p.kind == PK::Custom) hasCustom = true; }
        for (const ModelEntry &b : builtins()) { if (b.id == "b.deepseek" && hasDs) continue; if (b.id == "b.custom" && hasCustom) continue; out << b; }
        for (const ProviderCfg &p : provs) {
            ModelEntry e; e.id = "cfg:" + p.id; e.name = p.model.isEmpty() ? p.name : p.model; e.family = p.name; e.kind = p.kind; e.provId = p.id; e.apiId = p.model; out << e;
        }
        out += m_custom; return out;
    }
    ModelEntry byId(const QString &id) const { for (const ModelEntry &e : all()) if (e.id == id) return e; return ModelEntry(); }
    ModelEntry active() const {
        const QList<ModelEntry> l = all(); const QString id = Cfg::get("activeModel", "b.local").toString();
        for (const ModelEntry &e : l) if (e.id == id) return e;
        return l.first();
    }
    QString activeId() const { return active().id; }
    void setActive(const QString &id) { Cfg::set("activeModel", id); emit activeChanged(); emit changed(); }
    QString addCustom(ModelEntry e) { if (e.id.isEmpty()) e.id = "c:" + newId(); m_custom << e; persist(); emit changed(); return e.id; }
    void updateCustom(const ModelEntry &e) { for (ModelEntry &c : m_custom) if (c.id == e.id) c = e; persist(); emit changed(); }
    void removeCustom(const QString &id) { for (int i = 0; i < m_custom.size(); ++i) if (m_custom[i].id == id) { m_custom.removeAt(i); break; } if (Cfg::get("activeModel").toString() == id) setActive("b.local"); persist(); emit changed(); }
    ProviderCfg resolveProvider(const ModelEntry &e, bool *ok) const {
        if (ok) *ok = true;
        if (e.provId == "local" || (e.provId.isEmpty() && e.kind == PK::Local)) return LS()->cfg();
        if (!e.provId.isEmpty()) { bool f = false; const ProviderCfg c = AM()->find(e.provId, &f); if (f) return c; if (ok) *ok = false; return ProviderCfg(); }
        for (const ProviderCfg &p : AM()->providers()) if (p.kind == e.kind) return p;
        if (ok) *ok = false; return ProviderCfg();
    }
    QString apiModelId(const ModelEntry &e, const ProviderCfg &c) const { return e.apiId.isEmpty() ? c.model : e.apiId; }
    int visionOf(const ModelEntry &e, const ProviderCfg &c) const {
        if (e.vision >= 0) return e.vision;
        if (c.vision >= 0) return c.vision;
        return visionHeuristic(c.kind, apiModelId(e, c));
    }
    // seviye: 0 gri, 1 iyi, 2 kötü
    QString statusOf(const ModelEntry &e, int *level = nullptr) const {
        int lv = 0; QString t; bool ok = false; const ProviderCfg c = resolveProvider(e, &ok);
        if (!ok) { t = QStringLiteral("Yapılandırılmadı"); lv = 2; }
        else if (c.kind == PK::Local) { t = LS()->statusText(); lv = LS()->state() == 1 ? 1 : (LS()->state() == 2 ? 2 : 0); }
        else if (c.lastTested == 0) { t = QStringLiteral("Test edilmedi"); lv = 0; }
        else if (c.lastOk) { t = QStringLiteral("Bağlantı doğrulandı"); lv = 1; }
        else { t = QStringLiteral("Bağlantı hatası"); lv = 2; }
        if (level) *level = lv; return t;
    }
signals:
    void changed();
    void activeChanged();
private:
    void persist() { QJsonArray a; for (const ModelEntry &e : m_custom) a.append(e.toJson()); Cfg::set("customModels", QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact))); }
    QList<ModelEntry> m_custom;
};
static ModelCatalog *g_mc = nullptr;
static ModelCatalog *MC() { return g_mc; }

// ============================================================================
// 5) Sözdizimi renklendirme, Markdown, kod blokları, mesaj widget'ları
// ============================================================================
struct LangDef {
    QSet<QString> kw, types, lit; QStringList lineCm;
    bool cBlock = false, tripleQ = false, backtick = false, hashPre = false, ci = false, capTypes = false, dollarVar = false, charOnly = false, macros = false;
    int kind = 0;  // 0 genel, 1 json, 2 yaml, 3 html, 4 css
};
static QSet<QString> wordSet(const char *s) { QSet<QString> r; for (const QString &w : QString::fromLatin1(s).split(' ', Qt::SkipEmptyParts)) r.insert(w); return r; }
static QString canonLang(QString l) {
    l = l.toLower().trimmed(); const int sp = l.indexOf(QRegularExpression(QStringLiteral("[\\s{:]"))); if (sp > 0) l = l.left(sp);
    static const QHash<QString, QString> m{{"c++", "cpp"}, {"cc", "cpp"}, {"cxx", "cpp"}, {"hpp", "cpp"}, {"h", "c"}, {"js", "javascript"}, {"jsx", "javascript"}, {"mjs", "javascript"}, {"node", "javascript"},
        {"ts", "typescript"}, {"tsx", "typescript"}, {"py", "python"}, {"python3", "python"}, {"sh", "bash"}, {"shell", "bash"}, {"zsh", "bash"}, {"powershell", "bash"}, {"ps1", "bash"}, {"console", "bash"},
        {"yml", "yaml"}, {"xml", "html"}, {"svg", "html"}, {"htm", "html"}, {"rs", "rust"}, {"golang", "go"}, {"jsonc", "json"}, {"scss", "css"}, {"postgresql", "sql"}, {"mysql", "sql"}, {"sqlite", "sql"}};
    return m.value(l, l);
}
static QString langLabel(const QString &raw) {
    const QString c = canonLang(raw);
    static const QHash<QString, QString> n{{"cpp", "C++"}, {"c", "C"}, {"python", "Python"}, {"java", "Java"}, {"javascript", "JavaScript"}, {"typescript", "TypeScript"}, {"html", "HTML"}, {"css", "CSS"},
        {"json", "JSON"}, {"yaml", "YAML"}, {"bash", "Bash"}, {"sql", "SQL"}, {"rust", "Rust"}, {"go", "Go"}};
    if (n.contains(c)) return n.value(c);
    return raw.trimmed().isEmpty() ? QStringLiteral("Kod") : raw.trimmed();
}
static const LangDef *langDef(const QString &raw) {
    static QHash<QString, LangDef> defs; static bool init = false;
    if (!init) {
        init = true;
        LangDef py; py.kw = wordSet("and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield match case");
        py.lit = wordSet("True False None self cls"); py.types = wordSet("int str float bool list dict set tuple bytes object print len range type"); py.lineCm = {"#"}; py.tripleQ = true; defs["python"] = py;
        LangDef cp; cp.kw = wordSet("alignas alignof asm auto break case catch class const constexpr const_cast continue decltype default delete do dynamic_cast else enum explicit export extern final for friend goto if inline mutable namespace new noexcept operator override private protected public register reinterpret_cast return sizeof static static_assert static_cast struct switch template this throw try typedef typeid typename union using virtual volatile while concept requires co_await co_return co_yield");
        cp.types = wordSet("int long short char float double bool void signed unsigned size_t string vector map unordered_map set std wchar_t int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t");
        cp.lit = wordSet("true false nullptr NULL"); cp.lineCm = {"//"}; cp.cBlock = true; cp.hashPre = true; cp.charOnly = true; defs["cpp"] = cp;
        LangDef c = cp; c.kw = wordSet("auto break case const continue default do else enum extern for goto if inline register restrict return sizeof static struct switch typedef union volatile while"); c.lit = wordSet("NULL true false"); defs["c"] = c;
        LangDef jv; jv.kw = wordSet("abstract assert break case catch class const continue default do else enum extends final finally for goto if implements import instanceof interface native new package private protected public return static strictfp super switch synchronized this throw throws transient try var volatile while record");
        jv.types = wordSet("boolean byte char double float int long short void String Integer List Map Object System"); jv.lit = wordSet("true false null"); jv.lineCm = {"//"}; jv.cBlock = true; jv.charOnly = true; jv.capTypes = true; defs["java"] = jv;
        LangDef js; js.kw = wordSet("async await break case catch class const continue debugger default delete do else export extends finally for from function if import in instanceof let new of return static super switch this throw try typeof var void while with yield");
        js.lit = wordSet("true false null undefined NaN"); js.types = wordSet("console Promise Array Object Map Set JSON Math"); js.lineCm = {"//"}; js.cBlock = true; js.backtick = true; defs["javascript"] = js;
        LangDef ts = js; ts.kw += wordSet("interface type enum implements namespace declare abstract readonly private public protected as is keyof infer satisfies"); ts.types += wordSet("string number boolean any unknown never object bigint symbol"); ts.capTypes = true; defs["typescript"] = ts;
        LangDef js2; js2.kind = 1; js2.lit = wordSet("true false null"); defs["json"] = js2;
        LangDef ym; ym.kind = 2; ym.lit = wordSet("true false null yes no on off"); ym.lineCm = {"#"}; defs["yaml"] = ym;
        LangDef sh; sh.kw = wordSet("if then else elif fi for while do done case esac function in select until return exit export local readonly echo cd set unset source alias param foreach"); sh.lit = wordSet("true false"); sh.lineCm = {"#"}; sh.dollarVar = true; defs["bash"] = sh;
        LangDef sq; sq.kw = wordSet("select from where insert into values update set delete create table alter drop index view join inner outer left right full cross on as and or not null is in like between group by order having limit offset union all distinct case when then else end primary key foreign references default unique check constraint exists asc desc begin commit rollback"); sq.types = wordSet("int integer bigint smallint varchar char text boolean date timestamp datetime float double decimal numeric serial"); sq.lit = wordSet("true false"); sq.lineCm = {"--"}; sq.cBlock = true; sq.ci = true; defs["sql"] = sq;
        LangDef rs; rs.kw = wordSet("as async await break const continue crate dyn else enum extern fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait type unsafe use where while");
        rs.types = wordSet("i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 bool char str String Vec Option Result Box Some None Ok Err"); rs.lit = wordSet("true false"); rs.lineCm = {"//"}; rs.cBlock = true; rs.charOnly = true; rs.macros = true; rs.capTypes = true; defs["rust"] = rs;
        LangDef go; go.kw = wordSet("break case chan const continue default defer else fallthrough for func go goto if import interface map package range return select struct switch type var"); go.types = wordSet("int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 float32 float64 string bool byte rune error any");
        go.lit = wordSet("true false nil iota"); go.lineCm = {"//"}; go.cBlock = true; go.backtick = true; go.charOnly = true; defs["go"] = go;
        LangDef ht; ht.kind = 3; defs["html"] = ht; LangDef cs; cs.kind = 4; cs.cBlock = true; defs["css"] = cs;
    }
    auto it = defs.constFind(canonLang(raw)); return it == defs.constEnd() ? nullptr : &it.value();
}

class CodeHighlighter : public QSyntaxHighlighter {
public:
    CodeHighlighter(QTextDocument *d, const LangDef *def) : QSyntaxHighlighter(d), m_def(def) {
        refresh(); QObject::connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { refresh(); rehighlight(); });
    }
protected:
    void refresh() {
        const bool dk = g_t.dark;
        auto mk = [](const char *l, const char *d, bool dark, bool it = false) { QTextCharFormat f; f.setForeground(C(dark ? d : l)); if (it) f.setFontItalic(true); return f; };
        fKw = mk("#0000FF", "#569CD6", dk); fStr = mk("#A31515", "#CE9178", dk); fCom = mk("#008000", "#6A9955", dk, true); fNum = mk("#098658", "#B5CEA8", dk);
        fType = mk("#267F99", "#4EC9B0", dk); fFunc = mk("#795E26", "#DCDCAA", dk); fLit = mk("#0000FF", "#569CD6", dk); fPre = mk("#AF00DB", "#C586C0", dk);
        fKey = mk("#0451A5", "#9CDCFE", dk); fVar = mk("#001080", "#9CDCFE", dk);
    }
    static int findClose(const QString &t, int start, QChar q) {
        for (int j = start + 1; j < t.size(); ++j) { if (t[j] == '\\') { ++j; continue; } if (t[j] == q) return j; }
        return -1;
    }
    static QChar nextNonSpace(const QString &t, int j) { while (j < t.size() && t[j].isSpace()) ++j; return j < t.size() ? t[j] : QChar(); }
    void hlHtml(const QString &t) {
        int st = previousBlockState(); int i = 0; const int n = t.size();
        if (st == 5) { const int e = t.indexOf("-->"); if (e < 0) { setFormat(0, n, fCom); setCurrentBlockState(5); return; } setFormat(0, e + 3, fCom); i = e + 3; st = 0; }
        bool inTag = (st == 6);
        while (i < n) {
            if (!inTag) {
                const int lt = t.indexOf('<', i); if (lt < 0) break;
                if (t.mid(lt, 4) == "<!--") { const int e = t.indexOf("-->", lt + 4); if (e < 0) { setFormat(lt, n - lt, fCom); setCurrentBlockState(5); return; } setFormat(lt, e + 3 - lt, fCom); i = e + 3; continue; }
                int j = lt + 1; if (j < n && (t[j] == '/' || t[j] == '!' || t[j] == '?')) ++j;
                while (j < n && (t[j].isLetterOrNumber() || t[j] == ':' || t[j] == '-')) ++j;
                setFormat(lt, j - lt, fKw); i = j; inTag = true; continue;
            }
            if (t[i] == '>') { setFormat(i, 1, fKw); inTag = false; ++i; continue; }
            if (t[i] == '/' && i + 1 < n && t[i + 1] == '>') { setFormat(i, 2, fKw); inTag = false; i += 2; continue; }
            if (t[i] == '"' || t[i] == '\'') { int e = findClose(t, i, t[i]); if (e < 0) e = n - 1; setFormat(i, e + 1 - i, fStr); i = e + 1; continue; }
            if (t[i].isLetter()) { int j = i + 1; while (j < n && (t[j].isLetterOrNumber() || t[j] == '-' || t[j] == ':' || t[j] == '_' || t[j] == '@')) ++j; setFormat(i, j - i, fKey); i = j; continue; }
            ++i;
        }
        setCurrentBlockState(inTag ? 6 : 0);
    }
    void hlCss(const QString &t) {
        int st = previousBlockState(); int i = 0; const int n = t.size(); bool inB = (st == 7 || st == 8);
        if (st == 1 || st == 8) { const int e = t.indexOf("*/"); if (e < 0) { setFormat(0, n, fCom); setCurrentBlockState(st); return; } setFormat(0, e + 2, fCom); i = e + 2; }
        while (i < n) {
            const QChar c = t[i];
            if (c == '/' && i + 1 < n && t[i + 1] == '*') { const int e = t.indexOf("*/", i + 2); if (e < 0) { setFormat(i, n - i, fCom); setCurrentBlockState(inB ? 8 : 1); return; } setFormat(i, e + 2 - i, fCom); i = e + 2; continue; }
            if (c == '{') { inB = true; ++i; continue; }
            if (c == '}') { inB = false; ++i; continue; }
            if (c == '"' || c == '\'') { int e = findClose(t, i, c); if (e < 0) e = n - 1; setFormat(i, e + 1 - i, fStr); i = e + 1; continue; }
            if (c.isSpace() || c == ';' || c == ',' || c == ':' || c == '(' || c == ')') { ++i; continue; }
            if (!inB) { int j = i; while (j < n && t[j] != '{' && !(t[j] == '/' && j + 1 < n && t[j + 1] == '*')) ++j; setFormat(i, j - i, fType); i = j; continue; }
            if (c == '#') { int j = i + 1; while (j < n && (t[j].isDigit() || (t[j].toLower() >= 'a' && t[j].toLower() <= 'f'))) ++j; setFormat(i, j - i, fNum); i = j; continue; }
            if (c.isDigit()) { int j = i + 1; while (j < n && (t[j].isLetterOrNumber() || t[j] == '.' || t[j] == '%')) ++j; setFormat(i, j - i, fNum); i = j; continue; }
            if (c.isLetter() || c == '-' || c == '_') { int j = i + 1; while (j < n && (t[j].isLetterOrNumber() || t[j] == '-' || t[j] == '_')) ++j; if (nextNonSpace(t, j) == ':') setFormat(i, j - i, fKey); i = j; continue; }
            ++i;
        }
        setCurrentBlockState(inB ? 7 : 0);
    }
    void highlightBlock(const QString &t) override {
        if (!m_def) return;
        const LangDef &d = *m_def;
        if (d.kind == 3) { hlHtml(t); return; }
        if (d.kind == 4) { hlCss(t); return; }
        int st = previousBlockState(); if (st < 0) st = 0;
        int i = 0; const int n = t.size(); setCurrentBlockState(0);
        if (st == 1) { const int e = t.indexOf("*/"); if (e < 0) { setFormat(0, n, fCom); setCurrentBlockState(1); return; } setFormat(0, e + 2, fCom); i = e + 2; }
        else if (st == 2 || st == 3) { const QString q = st == 2 ? QStringLiteral("\"\"\"") : QStringLiteral("'''"); const int e = t.indexOf(q); if (e < 0) { setFormat(0, n, fStr); setCurrentBlockState(st); return; } setFormat(0, e + 3, fStr); i = e + 3; }
        else if (st == 4) { const int e = findClose(t, -1, '`'); if (e < 0) { setFormat(0, n, fStr); setCurrentBlockState(4); return; } setFormat(0, e + 1, fStr); i = e + 1; }
        while (i < n) {
            const QChar c = t[i];
            if (d.hashPre && c == '#' && t.left(i).trimmed().isEmpty()) { setFormat(i, n - i, fPre); return; }
            bool lc = false; for (const QString &p : d.lineCm) if (t.mid(i, p.size()) == p) { lc = true; break; }
            if (lc) { setFormat(i, n - i, fCom); return; }
            if (d.cBlock && c == '/' && i + 1 < n && t[i + 1] == '*') { const int e = t.indexOf("*/", i + 2); if (e < 0) { setFormat(i, n - i, fCom); setCurrentBlockState(1); return; } setFormat(i, e + 2 - i, fCom); i = e + 2; continue; }
            if (d.tripleQ && (t.mid(i, 3) == "\"\"\"" || t.mid(i, 3) == "'''")) {
                const QString q = t.mid(i, 3); const int e = t.indexOf(q, i + 3);
                if (e < 0) { setFormat(i, n - i, fStr); setCurrentBlockState(q[0] == '"' ? 2 : 3); return; }
                setFormat(i, e + 3 - i, fStr); i = e + 3; continue;
            }
            if (c == '"' || c == '\'' || (d.backtick && c == '`')) {
                const int e = findClose(t, i, c);
                if (e < 0) { if (c == '`') { setFormat(i, n - i, fStr); setCurrentBlockState(4); return; } if (c == '\'') { ++i; continue; } setFormat(i, n - i, fStr); return; }
                if (c == '\'' && d.charOnly && e - i > 4) { ++i; continue; }
                bool isKey = false; if (d.kind == 1 || d.kind == 2) isKey = nextNonSpace(t, e + 1) == ':';
                setFormat(i, e + 1 - i, isKey ? fKey : fStr); i = e + 1; continue;
            }
            if (c.isDigit() && (i == 0 || !(t[i - 1].isLetterOrNumber() || t[i - 1] == '_'))) { int j = i + 1; while (j < n && (t[j].isLetterOrNumber() || t[j] == '.' || t[j] == '_')) ++j; setFormat(i, j - i, fNum); i = j; continue; }
            if (d.dollarVar && c == '$') {
                int j = i + 1; if (j < n && t[j] == '{') { const int e = t.indexOf('}', j); j = e < 0 ? n : e + 1; } else while (j < n && (t[j].isLetterOrNumber() || t[j] == '_')) ++j;
                setFormat(i, j - i, fVar); i = j; continue;
            }
            if (c.isLetter() || c == '_') {
                int j = i + 1; while (j < n && (t[j].isLetterOrNumber() || t[j] == '_')) ++j;
                const QString w = t.mid(i, j - i); const QString k = d.ci ? w.toLower() : w;
                const QTextCharFormat *f = nullptr;
                if (d.kw.contains(k)) f = &fKw; else if (d.lit.contains(k)) f = &fLit; else if (d.types.contains(k)) f = &fType;
                else if (d.kind == 2 && nextNonSpace(t, j) == ':') f = &fKey;
                else if (d.macros && j < n && t[j] == '!') f = &fFunc;
                else if (j < n && t[j] == '(') f = &fFunc;
                else if (d.capTypes && w[0].isUpper()) f = &fType;
                if (f) setFormat(i, j - i, *f);
                i = j; continue;
            }
            ++i;
        }
    }
    const LangDef *m_def; QTextCharFormat fKw, fStr, fCom, fNum, fType, fFunc, fLit, fPre, fKey, fVar;
};

class CodeEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEdit(QWidget *p = nullptr) : QPlainTextEdit(p) {}
protected:
    void wheelEvent(QWheelEvent *e) override { if (qAbs(e->angleDelta().y()) >= qAbs(e->angleDelta().x())) e->ignore(); else QPlainTextEdit::wheelEvent(e); }
};

class CodeBlockWidget : public QFrame {
    Q_OBJECT
public:
    explicit CodeBlockWidget(QWidget *p = nullptr) : QFrame(p) {
        setObjectName("codeBox");
        auto *v = vb(this);
        m_head = new QFrame(this); m_head->setObjectName("codeHead"); m_head->setFixedHeight(38);
        auto *h = hb(m_head); h->setContentsMargins(14, 0, 8, 0); h->setSpacing(4);
        m_lang = new QLabel(m_head); m_lang->setProperty("role", "small");
        m_toggle = fb("chevup", QString(), m_head)->isz(16)->fixed(30, 28)->tip(QStringLiteral("Daralt / genişlet"));
        m_copy = fb("copy", QStringLiteral("Kopyala"), m_head)->isz(15)->fixed(96, 28);
        h->addWidget(m_lang); h->addStretch(1); h->addWidget(m_toggle); h->addWidget(m_copy);
        m_ed = new CodeEdit(this); m_ed->setObjectName("codeBody"); m_ed->setReadOnly(true); m_ed->setUndoRedoEnabled(false);
        m_ed->setLineWrapMode(QPlainTextEdit::NoWrap); m_ed->setFrameShape(QFrame::NoFrame);
        m_ed->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); m_ed->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_ed->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        v->addWidget(m_head); v->addWidget(m_ed);
        connect(m_copy, &QAbstractButton::clicked, this, [this] {
            QGuiApplication::clipboard()->setText(m_code); m_copy->setIconName("check"); m_copy->setText(QStringLiteral("Kopyalandı"));
            QTimer::singleShot(1500, this, [this] { m_copy->setIconName("copy"); m_copy->setText(QStringLiteral("Kopyala")); });
        });
        connect(m_toggle, &QAbstractButton::clicked, this, [this] { m_collapsed = !m_collapsed; m_ed->setVisible(!m_collapsed); m_toggle->setIconName(m_collapsed ? "chevdown" : "chevup"); });
    }
    void setCode(const QString &code, const QString &lang) {
        if (!m_init || lang != m_langKey) {
            m_init = true; m_langKey = lang; m_lang->setText(langLabel(lang));
            delete m_hl; m_hl = nullptr; m_ed->clear(); m_code.clear();
            m_hl = new CodeHighlighter(m_ed->document(), langDef(lang));
        }
        if (code != m_code) {
            if (!m_code.isEmpty() && code.startsWith(m_code)) { QTextCursor c(m_ed->document()); c.movePosition(QTextCursor::End); c.insertText(code.mid(m_code.size())); }
            else m_ed->setPlainText(code);
            m_code = code;
        }
        fit();
    }
    void fit() {
        m_ed->ensurePolished();
        m_ed->setTabStopDistance(4 * m_ed->fontMetrics().horizontalAdvance(' '));
        const int lines = qMax(1, m_ed->document()->blockCount());
        m_ed->setFixedHeight(lines * m_ed->fontMetrics().lineSpacing() + int(m_ed->document()->documentMargin() * 2) + 16);
    }
private:
    QFrame *m_head; QLabel *m_lang; FlatButton *m_toggle, *m_copy; CodeEdit *m_ed; CodeHighlighter *m_hl = nullptr; QString m_code, m_langKey; bool m_collapsed = false, m_init = false;
};

// ---------- Markdown -> HTML (Qt zengin metin alt kümesi) ----------
struct MdBlock { bool code = false, open = false; QString lang, text; };
static QList<MdBlock> mdSplit(const QString &src) {
    QList<MdBlock> out; QString text, code, lang, fence; bool inCode = false;
    auto flushText = [&] { if (!text.trimmed().isEmpty()) { MdBlock b; b.text = text; out.append(b); } text.clear(); };
    for (const QString &ln : src.split('\n')) {
        const QString t = ln.trimmed();
        if (!inCode) {
            if (t.startsWith("```") || t.startsWith("~~~")) { flushText(); inCode = true; fence = t.left(3); lang = t.mid(3).trimmed(); code.clear(); continue; }
            text += ln + '\n';
        } else {
            if (t.startsWith(fence) && t.mid(3).trimmed().isEmpty() && t.size() >= 3) {
                MdBlock b; b.code = true; b.lang = lang; b.text = code.endsWith('\n') ? code.left(code.size() - 1) : code; out.append(b); inCode = false; continue;
            }
            code += ln + '\n';
        }
    }
    if (inCode) { MdBlock b; b.code = true; b.open = true; b.lang = lang; b.text = code.endsWith('\n') ? code.left(code.size() - 1) : code; out.append(b); }
    else flushText();
    return out;
}
static QString mdInline(const QString &raw) {
    QStringList codes; QString s; int i = 0; const int n = raw.size();
    while (i < n) {
        if (raw[i] == '`') {
            int k = 1; while (i + k < n && raw[i + k] == '`') ++k;
            const QString fence(k, QLatin1Char('`')); const int e = raw.indexOf(fence, i + k);
            if (e > 0) { codes << raw.mid(i + k, e - i - k).trimmed(); s += QString("\x01%1\x02").arg(codes.size() - 1); i = e + k; continue; }
            s += fence; i += k; continue;
        }
        s += raw[i++];
    }
    s = s.toHtmlEscaped();
    static const QRegularExpression link(QStringLiteral("\\[([^\\]]+)\\]\\((https?://[^\\s)]+)\\)")), bare(QStringLiteral("(?<![\">=/])(https?://[^\\s<]+[^\\s<.,;:!?)])"));
    static const QRegularExpression b1(QStringLiteral("\\*\\*(.+?)\\*\\*")), b2(QStringLiteral("__(.+?)__")), st(QStringLiteral("~~(.+?)~~"));
    static const QRegularExpression i1(QStringLiteral("(?<![\\*\\w])\\*(?![\\s\\*])(.+?)(?<![\\s\\*])\\*(?![\\*\\w])")), i2(QStringLiteral("(?<![_\\w])_(?![\\s_])(.+?)(?<![\\s_])_(?![_\\w])"));
    const QString ls = QStringLiteral("style=\"color:") + g_t.accent.name() + QStringLiteral(";text-decoration:none;\"");
    s.replace(link, QStringLiteral("<a href=\"\\2\" ") + ls + QStringLiteral(">\\1</a>"));
    s.replace(bare, QStringLiteral("<a href=\"\\1\" ") + ls + QStringLiteral(">\\1</a>"));
    s.replace(b1, QStringLiteral("<b>\\1</b>")); s.replace(b2, QStringLiteral("<b>\\1</b>"));
    s.replace(i1, QStringLiteral("<i>\\1</i>")); s.replace(i2, QStringLiteral("<i>\\1</i>")); s.replace(st, QStringLiteral("<s>\\1</s>"));
    for (int k = 0; k < codes.size(); ++k) {
        const QString span = QStringLiteral("<span style=\"font-family:Consolas,'Cascadia Mono',monospace;font-size:13px;background-color:") + g_t.hover.name() + QStringLiteral(";\">&nbsp;") + codes[k].toHtmlEscaped() + QStringLiteral("&nbsp;</span>");
        s.replace(QString("\x01%1\x02").arg(k), span);
    }
    return s;
}
static bool mdIsTableSep(const QString &l) {
    static const QRegularExpression re(QStringLiteral("^\\s*\\|?\\s*:?-{2,}:?\\s*(\\|\\s*:?-{2,}:?\\s*)*\\|?\\s*$"));
    return l.contains('|') && re.match(l).hasMatch();
}
static QStringList mdRow(QString l) { l = l.trimmed(); if (l.startsWith('|')) l.remove(0, 1); if (l.endsWith('|')) l.chop(1); QStringList c = l.split('|'); for (QString &x : c) x = x.trimmed(); return c; }
static QString mdToHtml(const QString &src) {
    const QStringList L = src.split('\n'); const int n = L.size(); QString out; int i = 0;
    static const QRegularExpression hRe(QStringLiteral("^\\s{0,3}(#{1,6})\\s+(.*?)\\s*#*\\s*$")), hrRe(QStringLiteral("^\\s{0,3}([-*_])(\\s*\\1){2,}\\s*$")), liRe(QStringLiteral("^(\\s*)([-*+]|\\d+[.)])\\s+(.*)$"));
    const QString lh = QStringLiteral("line-height:145%;");
    while (i < n) {
        const QString &ln = L[i]; const QString t = ln.trimmed();
        if (t.isEmpty()) { ++i; continue; }
        QRegularExpressionMatch hm = hRe.match(ln);
        if (hm.hasMatch()) {
            static const int sz[6] = {22, 19, 17, 16, 15, 15};
            out += QStringLiteral("<p style=\"margin-top:14px;margin-bottom:6px;\"><span style=\"font-size:") + QString::number(sz[hm.captured(1).size() - 1]) + QStringLiteral("px;font-weight:600;\">") + mdInline(hm.captured(2)) + QStringLiteral("</span></p>");
            ++i; continue;
        }
        if (hrRe.match(ln).hasMatch()) { out += QStringLiteral("<hr/>"); ++i; continue; }
        if (t.startsWith('>')) {
            QStringList inner; static const QRegularExpression q(QStringLiteral("^\\s*>\\s?"));
            while (i < n && L[i].trimmed().startsWith('>')) { QString x = L[i]; x.remove(q); inner << x; ++i; }
            out += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"0\" width=\"100%\" style=\"margin-top:4px;margin-bottom:10px;\"><tr><td width=\"4\" bgcolor=\"") + g_t.border.darker(115).name() + QStringLiteral("\">&nbsp;</td><td style=\"padding-left:12px;color:") + g_t.text2.name() + QStringLiteral(";\">") + mdToHtml(inner.join("\n")) + QStringLiteral("</td></tr></table>");
            continue;
        }
        if (ln.contains('|') && i + 1 < n && mdIsTableSep(L[i + 1])) {
            const QStringList head = mdRow(ln), sep = mdRow(L[i + 1]); QStringList al;
            for (const QString &s : sep) al << (s.startsWith(':') && s.endsWith(':') ? "center" : (s.endsWith(':') ? "right" : "left"));
            i += 2; QList<QStringList> rows;
            while (i < n && !L[i].trimmed().isEmpty() && L[i].contains('|')) { rows << mdRow(L[i]); ++i; }
            QString h = QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"7\" style=\"border-collapse:collapse;border-color:") + g_t.border.darker(108).name() + QStringLiteral(";margin-top:6px;margin-bottom:12px;\"><tr>");
            for (int c = 0; c < head.size(); ++c) h += QStringLiteral("<th align=\"") + (c < al.size() ? al[c] : QStringLiteral("left")) + QStringLiteral("\" bgcolor=\"") + g_t.hover.name() + QStringLiteral("\" style=\"font-weight:600;\">") + mdInline(head[c]) + QStringLiteral("</th>");
            h += QStringLiteral("</tr>");
            for (const QStringList &r : rows) { h += QStringLiteral("<tr>"); for (int c = 0; c < head.size(); ++c) h += QStringLiteral("<td align=\"") + (c < al.size() ? al[c] : QStringLiteral("left")) + QStringLiteral("\">") + (c < r.size() ? mdInline(r[c]) : QString()) + QStringLiteral("</td>"); h += QStringLiteral("</tr>"); }
            out += h + QStringLiteral("</table>"); continue;
        }
        if (liRe.match(ln).hasMatch()) {
            struct It { int ind; bool ord; int num; QString txt; }; QList<It> items;
            auto indentOf = [](const QString &s) { int w = 0; for (QChar c : s) w += (c == '\t') ? 4 : 1; return w; };
            while (i < n) {
                QRegularExpressionMatch m = liRe.match(L[i]);
                if (m.hasMatch()) { It it{indentOf(m.captured(1)), m.captured(2)[0].isDigit(), m.captured(2).toInt(), m.captured(3)}; items << it; ++i; continue; }
                if (!L[i].trimmed().isEmpty() && L[i].startsWith("  ") && !items.isEmpty() && !hRe.match(L[i]).hasMatch()) { items.last().txt += "\n" + L[i].trimmed(); ++i; continue; }
                if (L[i].trimmed().isEmpty()) { int k = i + 1; while (k < n && L[k].trimmed().isEmpty()) ++k; if (k < n && liRe.match(L[k]).hasMatch()) { i = k; continue; } }
                break;
            }
            QList<int> inds; QList<bool> ords; QString h;
            auto openTag = [&](const It &it) { return it.ord ? QStringLiteral("<ol start=\"") + QString::number(qMax(1, it.num)) + QStringLiteral("\" style=\"margin-top:2px;margin-bottom:8px;\">") : QStringLiteral("<ul style=\"margin-top:2px;margin-bottom:8px;\">"); };
            for (const It &it : items) {
                while (!inds.isEmpty() && it.ind < inds.last()) { h += ords.last() ? "</ol>" : "</ul>"; inds.removeLast(); ords.removeLast(); }
                if (inds.isEmpty() || it.ind > inds.last()) { h += openTag(it); inds << it.ind; ords << it.ord; }
                else if (ords.last() != it.ord) { h += ords.last() ? "</ol>" : "</ul>"; h += openTag(it); ords.last() = it.ord; }
                h += QStringLiteral("<li style=\"margin-top:2px;margin-bottom:2px;") + lh + QStringLiteral("\">") + mdInline(it.txt).replace("\n", "<br>") + QStringLiteral("</li>");
            }
            while (!inds.isEmpty()) { h += ords.last() ? "</ol>" : "</ul>"; inds.removeLast(); ords.removeLast(); }
            out += h; continue;
        }
        QStringList para;
        while (i < n) {
            const QString &x = L[i]; if (x.trimmed().isEmpty()) break;
            if (!para.isEmpty() && (hRe.match(x).hasMatch() || hrRe.match(x).hasMatch() || liRe.match(x).hasMatch() || x.trimmed().startsWith('>') || (x.contains('|') && i + 1 < n && mdIsTableSep(L[i + 1])))) break;
            para << x; ++i;
        }
        out += QStringLiteral("<p style=\"margin-top:0px;margin-bottom:10px;") + lh + QStringLiteral("\">") + mdInline(para.join("\n")).replace("\n", "<br>") + QStringLiteral("</p>");
    }
    return out;
}

class MarkdownView : public QWidget {
    Q_OBJECT
public:
    explicit MarkdownView(QWidget *p = nullptr) : QWidget(p) {
        m_lay = vb(this, 0, 4);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { rebuild(true); });
    }
    const QString &markdown() const { return m_md; }
    void setMarkdown(const QString &md, bool streaming = false) { if (md == m_md && streaming == m_stream) return; m_md = md; m_stream = streaming; rebuild(false); }
private:
    struct Item { bool code; QString key; QWidget *w; };
    QString html(const QString &text, bool caret) const {
        QString h = mdToHtml(text);
        if (caret) { const QString c = QStringLiteral("<span style=\"color:") + g_t.accent.name() + QStringLiteral(";\"> ▍</span>"); const int k = h.lastIndexOf("</p>"); if (k >= 0) h.insert(k, c); else h += c; }
        return h;
    }
    void rebuild(bool force) {
        const QList<MdBlock> bl = mdSplit(m_md);
        for (int idx = 0; idx < bl.size(); ++idx) {
            const MdBlock &b = bl[idx]; const bool last = idx == bl.size() - 1;
            while (idx < m_items.size() && m_items[idx].code != b.code) { delete m_items[idx].w; m_items.removeAt(idx); }
            if (idx < m_items.size()) {
                Item &it = m_items[idx];
                if (b.code) static_cast<CodeBlockWidget *>(it.w)->setCode(b.text, b.lang);
                else {
                    const QString key = b.text + ((last && m_stream) ? QString(QChar(3)) : QString());
                    if (force || it.key != key) { static_cast<QLabel *>(it.w)->setText(html(b.text, last && m_stream)); it.key = key; }
                }
                continue;
            }
            QWidget *w;
            if (b.code) { auto *cb = new CodeBlockWidget(this); cb->setCode(b.text, b.lang); w = cb; m_items.insert(idx, Item{true, QString(), w}); }
            else {
                auto *l = new QLabel(this); l->setProperty("role", "msg"); l->setWordWrap(true); l->setTextFormat(Qt::RichText); l->setOpenExternalLinks(true);
                l->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse); l->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
                l->setText(html(b.text, last && m_stream)); w = l; m_items.insert(idx, Item{false, b.text + ((last && m_stream) ? QString(QChar(3)) : QString()), w});
            }
            m_lay->insertWidget(idx, w);
        }
        while (m_items.size() > bl.size()) delete m_items.takeLast().w;
    }
    QVBoxLayout *m_lay; QList<Item> m_items; QString m_md; bool m_stream = false;
};

static void animOpacity(QGraphicsOpacityEffect *e, qreal to, int ms = 140) {
    if (!Cfg::anim()) { e->setOpacity(to); return; }
    auto *a = new QPropertyAnimation(e, "opacity", e); a->setDuration(ms); a->setStartValue(e->opacity()); a->setEndValue(to); a->start(QAbstractAnimation::DeleteWhenStopped);
}

class MessageWidget : public QWidget {
    Q_OBJECT
public:
    MessageWidget(const MsgRec &m, QWidget *parent = nullptr) : QWidget(parent), m_id(m.id), m_role(m.role), m_text(m.content), m_fb(m.feedback) {
        if (m_role == QLatin1String("user")) buildUser(m); else buildAssistant(m);
    }
    qint64 dbId() const { return m_id; }
    void setDbId(qint64 i) { m_id = i; }
    bool isUser() const { return m_role == QLatin1String("user"); }
    QString text() const { return m_text; }
    void playAppear() { if (Cfg::anim()) fadeWidget(this, true, 240); }
    void showTyping(bool on) { if (m_typing) m_typing->setVisible(on); }
    void setStreaming(const QString &md) { m_text = md; if (m_typing) m_typing->hide(); m_md->setMarkdown(md, true); }
    void finish(const QString &md) {
        m_text = md; if (m_typing) m_typing->hide(); m_md->setMarkdown(md, false);
        m_act->setVisible(!md.isEmpty()); setLast(m_isLast); applyFeedback();
    }
    void setError(const QString &msg, bool retry, bool settings) {
        if (m_typing) m_typing->hide();
        if (!m_errBox) {
            m_errBox = new QFrame(this); m_errBox->setObjectName("errBox");
            auto *v = vb(m_errBox, 12, 8); m_errLbl = new QLabel(m_errBox); m_errLbl->setProperty("role", "err"); m_errLbl->setWordWrap(true);
            m_errLbl->setTextInteractionFlags(Qt::TextSelectableByMouse); v->addWidget(m_errLbl);
            m_errRow = new QWidget(m_errBox); auto *rh = hb(m_errRow, 0, 8);
            m_retryBtn = fb("refresh", QStringLiteral("Tekrar dene"), m_errRow)->kind(FlatButton::Outline)->isz(16)->fixed(130, 32);
            m_setBtn = fb("settings", QStringLiteral("API ayarlarını aç"), m_errRow)->kind(FlatButton::Outline)->isz(16)->fixed(170, 32);
            rh->addWidget(m_retryBtn); rh->addWidget(m_setBtn); rh->addStretch(1); v->addWidget(m_errRow);
            connect(m_retryBtn, &QAbstractButton::clicked, this, &MessageWidget::retry);
            connect(m_setBtn, &QAbstractButton::clicked, this, &MessageWidget::openSettings);
            static_cast<QVBoxLayout *>(layout())->insertWidget(2, m_errBox);
        }
        m_errLbl->setText(msg); m_retryBtn->setVisible(retry); m_setBtn->setVisible(settings); m_errRow->setVisible(retry || settings); m_errBox->show();
    }
    void setLast(bool l) { m_isLast = l; if (m_regen) m_regen->setVisible(l); }
    void setFeedback(int f) { m_fb = f; applyFeedback(); }
signals:
    void regenerate();
    void retry();
    void openSettings();
    void feedbackChanged(qint64 id, int f);
    void deleteRequested();
protected:
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e); relayoutBubble(); }
    void showEvent(QShowEvent *e) override { QWidget::showEvent(e); relayoutBubble(); }
    void enterEvent(QEnterEvent *e) override { QWidget::enterEvent(e); if (m_uEff) animOpacity(m_uEff, 1.0); }
    void leaveEvent(QEvent *e) override { QWidget::leaveEvent(e); if (m_uEff) animOpacity(m_uEff, 0.0); }
private:
    FlatButton *actBtn(const QString &icon, const QString &tip, QWidget *p) { return fb(icon, QString(), p)->isz(17)->fixed(32, 30)->tip(tip); }
    void copyText(FlatButton *b) {
        QGuiApplication::clipboard()->setText(m_text);
        b->setIconName("check"); QTimer::singleShot(1200, b, [b] { b->setIconName("copy"); });
        toast(this, QStringLiteral("Panoya kopyalandı"));
    }
    void buildUser(const MsgRec &m) {
        auto *h = hb(this); h->addStretch(1);
        auto *col = new QWidget(this); auto *v = vb(col, 0, 6);
        if (!m.images.isEmpty()) {
            auto *ir = new QWidget(col); auto *ih = hb(ir, 0, 8);
            for (const QString &p : m.images) {
                auto *ri = new RoundImage(thumbPix(p, 110), QSize(110, 110), 12, QFileInfo(p).exists() ? QFileInfo(p).fileName() : QStringLiteral("Dosya bulunamadı"), ir);
                if (QFileInfo(p).exists()) { ri->setClickable(true); connect(ri, &RoundImage::clicked, this, [p] { QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }); }
                ih->addWidget(ri);
            }
            v->addWidget(ir, 0, Qt::AlignRight);
        }
        if (!m.content.isEmpty()) {
            m_bubble = new QFrame(col); m_bubble->setObjectName("bubble");
            auto *bl = vb(m_bubble); bl->setContentsMargins(16, 10, 16, 10);
            m_lbl = new QLabel(m.content, m_bubble); m_lbl->setProperty("role", "msg"); m_lbl->setWordWrap(true); m_lbl->setTextFormat(Qt::PlainText);
            m_lbl->setTextInteractionFlags(Qt::TextSelectableByMouse); bl->addWidget(m_lbl);
            v->addWidget(m_bubble, 0, Qt::AlignRight);
            m_uAct = new QWidget(col); auto *ah = hb(m_uAct, 0, 2); ah->addStretch(1);
            auto *cb = actBtn("copy", QStringLiteral("Kopyala"), m_uAct); ah->addWidget(cb);
            connect(cb, &QAbstractButton::clicked, this, [this, cb] { copyText(cb); });
            m_uEff = new QGraphicsOpacityEffect(m_uAct); m_uEff->setOpacity(0.0); m_uAct->setGraphicsEffect(m_uEff);
            v->addWidget(m_uAct, 0, Qt::AlignRight);
        }
        h->addWidget(col);
    }
    void buildAssistant(const MsgRec &m) {
        auto *v = vb(this, 0, 6);
        m_typing = new TypingIndicator(this); m_typing->hide();
        m_md = new MarkdownView(this); m_md->setMarkdown(m.content, false);
        m_act = new QWidget(this); auto *ah = hb(m_act, 0, 2);
        auto *cp = actBtn("copy", QStringLiteral("Yanıtı kopyala"), m_act);
        m_like = actBtn("thumbup", QStringLiteral("İyi yanıt"), m_act); m_dis = actBtn("thumbdown", QStringLiteral("Kötü yanıt"), m_act);
        m_regen = actBtn("refresh", QStringLiteral("Yeniden oluştur"), m_act); auto *more = actBtn("more", QStringLiteral("Diğer seçenekler"), m_act);
        ah->addWidget(cp); ah->addWidget(m_like); ah->addWidget(m_dis); ah->addWidget(m_regen); ah->addWidget(more); ah->addStretch(1);
        v->addWidget(m_typing); v->addWidget(m_md); v->addWidget(m_act);
        m_act->setVisible(!m.content.isEmpty()); m_regen->setVisible(false); applyFeedback();
        connect(cp, &QAbstractButton::clicked, this, [this, cp] { copyText(cp); });
        connect(m_like, &QAbstractButton::clicked, this, [this] { m_fb = (m_fb == 1) ? 0 : 1; applyFeedback(); emit feedbackChanged(m_id, m_fb); });
        connect(m_dis, &QAbstractButton::clicked, this, [this] { m_fb = (m_fb == -1) ? 0 : -1; applyFeedback(); emit feedbackChanged(m_id, m_fb); });
        connect(m_regen, &QAbstractButton::clicked, this, &MessageWidget::regenerate);
        connect(more, &QAbstractButton::clicked, this, [this, more] {
            QMenu menu(this);
            QAction *a1 = menu.addAction(QIcon(iconPix("copy", 18, g_t.text)), QStringLiteral("Metin olarak kopyala"));
            QAction *a2 = menu.addAction(QIcon(iconPix("trash", 18, g_t.danger)), QStringLiteral("Bu yanıtı sil"));
            QAction *r = menu.exec(more->mapToGlobal(QPoint(0, more->height() + 4)));
            if (r == a1) { QGuiApplication::clipboard()->setText(m_text); toast(this, QStringLiteral("Panoya kopyalandı")); }
            else if (r == a2) emit deleteRequested();
        });
    }
    void applyFeedback() {
        if (!m_like) return;
        m_like->tint(m_fb == 1 ? g_t.accent : QColor()); m_dis->tint(m_fb == -1 ? g_t.accent : QColor());
        m_like->setIconName("thumbup"); m_dis->setIconName("thumbdown");
    }
    void relayoutBubble() {
        if (!m_lbl) return;
        m_lbl->ensurePolished();
        const int maxW = qMax(160, int(width() * 0.80) - 36);
        const QRect r = QFontMetrics(m_lbl->font()).boundingRect(QRect(0, 0, maxW, 100000), Qt::TextWordWrap | Qt::AlignLeft, m_text);
        m_lbl->setFixedWidth(qMin(maxW, r.width() + 6));
    }
    qint64 m_id; QString m_role, m_text; int m_fb = 0; bool m_isLast = false;
    QFrame *m_bubble = nullptr; QLabel *m_lbl = nullptr; QWidget *m_uAct = nullptr; QGraphicsOpacityEffect *m_uEff = nullptr;
    TypingIndicator *m_typing = nullptr; MarkdownView *m_md = nullptr; QWidget *m_act = nullptr; FlatButton *m_like = nullptr, *m_dis = nullptr, *m_regen = nullptr;
    QFrame *m_errBox = nullptr; QLabel *m_errLbl = nullptr; QWidget *m_errRow = nullptr; FlatButton *m_retryBtn = nullptr, *m_setBtn = nullptr;
};

// ============================================================================
// 6) Donanım paneli, model seçici, model düzenleme
// ============================================================================
static QString fmtMB(qint64 mb) { if (mb < 0) return QStringLiteral("Unavailable"); return mb >= 1024 ? QString::number(mb / 1024.0, 'f', 1) + " GB" : QString::number(mb) + " MB"; }
static QString fmtGB(double g) { return QString::number(g, 'f', 1) + " GB"; }
static QString fmtParams(double b) { if (b <= 0) return QStringLiteral("Bilinmiyor"); return (b == std::floor(b) ? QString::number(int(b)) : QString::number(b, 'f', 1)) + "B"; }

class HardwarePanel : public QWidget {
    Q_OBJECT
public:
    explicit HardwarePanel(bool compact, QWidget *p = nullptr) : QWidget(p), m_compact(compact) {
        m_lay = vb(this, 0, compact ? 10 : 18);
        if (!compact) m_cpu = mk(QStringLiteral("CPU"));
        m_ram = mk(QStringLiteral("RAM"));
        if (compact) m_vram = mk(QStringLiteral("VRAM"));
        if (!compact) {
            m_gpuHost = new QWidget(this); m_gpuLay = vb(m_gpuHost, 0, 18); m_lay->addWidget(m_gpuHost);
            auto *note = mkLabel(QStringLiteral("GPU/VRAM bilgisi yalnızca NVIDIA için (nvidia-smi ile) okunur. Okunamayan değerler “Unavailable” olarak gösterilir. Değerler her 2 saniyede bir yenilenir."), "small", this);
            note->setWordWrap(true); m_lay->addWidget(note);
        }
        connect(HW(), &HardwareMonitor::updated, this, [this] { if (isVisible()) refresh(); });
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { refresh(); });
    }
    ~HardwarePanel() override { if (m_acq) HW()->release(); }
protected:
    void showEvent(QShowEvent *e) override { QWidget::showEvent(e); if (!m_acq) { m_acq = true; HW()->acquire(); } refresh(); }
    void hideEvent(QHideEvent *e) override { QWidget::hideEvent(e); if (m_acq) { m_acq = false; HW()->release(); } }
private:
    struct Meter { QWidget *box = nullptr; QLabel *head = nullptr, *detail = nullptr; QProgressBar *bar = nullptr; };
    Meter mk(const QString &title, bool add = true) {
        Meter m; m.box = new QWidget(this); auto *v = vb(m.box, 0, 5);
        m.head = new QLabel(title, m.box); m.head->setProperty("role", m_compact ? "small" : "h2");
        m.bar = new QProgressBar(m.box); m.bar->setRange(0, 1000); m.bar->setTextVisible(false);
        m.detail = new QLabel(m.box); m.detail->setProperty("role", m_compact ? "small" : "muted"); m.detail->setWordWrap(true);
        v->addWidget(m.head); v->addWidget(m.bar); v->addWidget(m.detail);
        if (add) m_lay->addWidget(m.box);
        return m;
    }
    void refresh() {
        const HwSnap &s = HW()->snap();
        if (m_cpu.box) {
            m_cpu.head->setText(QStringLiteral("CPU — ") + s.cpuName);
            m_cpu.bar->setValue(s.cpuUsage < 0 ? 0 : int(s.cpuUsage * 10));
            m_cpu.detail->setText(QStringLiteral("Kullanım: ") + (s.cpuUsage < 0 ? QStringLiteral("Unavailable") : QString::number(s.cpuUsage, 'f', 0) + " %") + QStringLiteral("   ·   Mantıksal çekirdek: ") + QString::number(s.cores));
        }
        m_ram.bar->setValue(s.ramTotal > 0 ? int(1000.0 * s.ramUsed / s.ramTotal) : 0);
        m_ram.detail->setText(s.ramTotal > 0 ? QStringLiteral("Toplam: %1   ·   Kullanılan: %2   ·   Kullanılabilir: %3").arg(fmtBytes(s.ramTotal), fmtBytes(s.ramUsed), fmtBytes(s.ramAvail)) : QStringLiteral("Unavailable"));
        if (m_compact) {
            if (HW()->vramTotalGB() > 0) {
                const double tot = HW()->vramTotalGB(), fr = HW()->vramFreeGB();
                m_vram.bar->setValue(int(1000.0 * (tot - fr) / tot));
                m_vram.detail->setText(QStringLiteral("Toplam: %1   ·   Boş: %2").arg(fmtGB(tot), fmtGB(fr)));
            } else { m_vram.bar->setValue(0); m_vram.detail->setText(QStringLiteral("Unavailable")); }
            return;
        }
        const int want = s.gpuOk ? s.gpus.size() : 1;
        if (m_gpus.size() != want) {
            for (const Meter &g : m_gpus) delete g.box;
            m_gpus.clear();
            for (int i = 0; i < want; ++i) { Meter g = mk(QStringLiteral("GPU"), false); m_gpuLay->addWidget(g.box); m_gpus << g; }
        }
        if (!s.gpuOk) { m_gpus[0].head->setText(QStringLiteral("GPU")); m_gpus[0].bar->setValue(0); m_gpus[0].detail->setText(s.gpuMsg.isEmpty() ? QStringLiteral("Unavailable") : s.gpuMsg); return; }
        for (int i = 0; i < want; ++i) {
            const GpuInfo &g = s.gpus[i];
            m_gpus[i].head->setText(QStringLiteral("GPU %1 — %2").arg(i).arg(g.name));
            m_gpus[i].bar->setValue(g.totalMB > 0 && g.usedMB >= 0 ? int(1000.0 * g.usedMB / g.totalMB) : 0);
            m_gpus[i].detail->setText(QStringLiteral("VRAM toplam: %1   ·   Kullanılan: %2   ·   Boş: %3   ·   GPU kullanımı: %4").arg(fmtMB(g.totalMB), fmtMB(g.usedMB), fmtMB(g.freeMB), g.util < 0 ? QStringLiteral("Unavailable") : QString::number(g.util) + " %"));
        }
    }
    bool m_compact, m_acq = false; QVBoxLayout *m_lay, *m_gpuLay = nullptr; QWidget *m_gpuHost = nullptr; Meter m_cpu, m_ram, m_vram; QList<Meter> m_gpus;
};

// ---------- model ayrıntı metni ----------
static QString memSummary(const ModelEntry &e) {
    if (e.kind != PK::Local) return QStringLiteral("Uygulanamaz (model sağlayıcı tarafında çalışır).");
    const MemEst m = estimateMem(e);
    if (!m.known) return QStringLiteral("Bilinmiyor — tahmin için parametre sayısı ve kuantizasyon gerekir.");
    if (m.demo) return QStringLiteral("Örnek (demo) değerler: VRAM %1, RAM %2. Gerçek ölçüm değildir.").arg(fmtGB(m.vram), fmtGB(m.ram));
    QString s = QStringLiteral("≈ %1 (tahmin) = ağırlıklar ≈ %2").arg(fmtGB(m.vram), fmtGB(m.weights));
    s += m.kv > 0 ? QStringLiteral(" + KV önbelleği ≈ %1").arg(fmtGB(m.kv)) : QStringLiteral(" + KV önbelleği (mimari bilgisi yok, dahil değil)");
    s += QStringLiteral(" + çalışma payı ≈ %1 (sabit tahmin)").arg(fmtGB(m.overhead));
    return s;
}
static QString modelDetailHtml(const ModelEntry &e) {
    bool ok = false; const ProviderCfg c = MC()->resolveProvider(e, &ok);
    int lv = 0; const QString st = MC()->statusOf(e, &lv);
    const QColor stc = lv == 1 ? g_t.ok : (lv == 2 ? g_t.danger : g_t.text2);
    const Fit ft = fitOf(e); const MemEst m = estimateMem(e);
    const QString k = g_t.text2.name(), v = g_t.text.name();
    auto row = [&](const QString &a, const QString &b, const QString &col = QString()) {
        return QStringLiteral("<tr><td style=\"color:%1;padding-right:12px;\" valign=\"top\">%2</td><td style=\"color:%3;\">%4</td></tr>").arg(k, a.toHtmlEscaped(), col.isEmpty() ? v : col, b.toHtmlEscaped());
    };
    QString h = QStringLiteral("<p style=\"margin:0 0 6px 0;font-size:15px;font-weight:600;\">%1</p><table cellspacing=\"0\" cellpadding=\"3\">").arg(e.name.toHtmlEscaped());
    h += row(QStringLiteral("Sağlayıcı"), ok ? c.name : pkName(e.kind));
    h += row(QStringLiteral("Konum"), e.location());
    h += row(QStringLiteral("Parametre"), fmtParams(e.paramsB));
    h += row(QStringLiteral("Kuantizasyon"), e.kind != PK::Local ? QStringLiteral("Uygulanamaz") : (e.quant.isEmpty() ? QStringLiteral("Bilinmiyor") : e.quant));
    h += row(QStringLiteral("Bağlam uzunluğu"), e.ctx > 0 ? QString::number(e.ctx) : QStringLiteral("Bilinmiyor"));
    if (e.kind == PK::Local) {
        h += row(QStringLiteral("Tahmini VRAM"), m.known ? (m.demo ? fmtGB(m.vram) + " (örnek)" : "≈ " + fmtGB(m.vram) + " (tahmin)") : QStringLiteral("Bilinmiyor"));
        h += row(QStringLiteral("Tahmini RAM"), m.known ? (m.demo ? fmtGB(m.ram) + " (örnek)" : "≈ " + fmtGB(m.ram) + " (tahmin, yalnızca CPU)") : QStringLiteral("Bilinmiyor"));
        h += row(QStringLiteral("Ayrıntı"), memSummary(e));
    } else { h += row(QStringLiteral("Tahmini VRAM / RAM"), QStringLiteral("Uygulanamaz (bulut/API)")); }
    h += row(QStringLiteral("Uyumluluk"), fitText(ft), fitColor(ft).name());
    h += row(QStringLiteral("Bağlantı durumu"), st, stc.name());
    const QString am = ok ? MC()->apiModelId(e, c) : e.apiId;
    h += row(QStringLiteral("İstekte gönderilen model"), am.isEmpty() ? QStringLiteral("(henüz seçilmedi)") : am);
    const int vis = ok ? MC()->visionOf(e, c) : -1;
    h += row(QStringLiteral("Görsel desteği"), vis == 1 ? QStringLiteral("Evet") : (vis == 0 ? QStringLiteral("Hayır") : QStringLiteral("Bilinmiyor (gönderilirse sunucu karar verir)")));
    h += QStringLiteral("</table>");
    QString note;
    if (e.demo) note = QStringLiteral("Bu giriş yalnızca gösterim amaçlıdır; değerler gerçek bir modele ait değildir.");
    else if (e.kind == PK::Local) note = QStringLiteral("Bellek değerleri model dosyasından okunmaz; parametre, kuantizasyon ve bağlam bilgisine dayalı tahmindir. Ağırlık boyutu ile toplam çalışma belleği farklıdır.");
    if (e.builtin && e.kind == PK::Local && e.provId == "local" && e.paramsB > 0) note += QStringLiteral(" Bu bir örnek profildir: sohbet istekleri RÜZGAR Local Server'ın seçili modeline gider.");
    if (!note.isEmpty()) h += QStringLiteral("<p style=\"margin-top:8px;color:%1;font-size:12px;\">%2</p>").arg(k, note.toHtmlEscaped());
    return h;
}

class ModelRow : public QAbstractButton {
    Q_OBJECT
public:
    ModelRow(const ModelEntry &e, bool active, QWidget *p = nullptr) : QAbstractButton(p), m_e(e), m_active(active) {
        setCursor(Qt::PointingHandCursor); setFocusPolicy(Qt::NoFocus); setFixedHeight(54);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
    const ModelEntry &entry() const { return m_e; }
signals:
    void hovered(const ModelEntry &e);
protected:
    void enterEvent(QEnterEvent *ev) override { QAbstractButton::enterEvent(ev); emit hovered(m_e); update(); }
    void leaveEvent(QEvent *ev) override { QAbstractButton::leaveEvent(ev); update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        if (underMouse()) { p.setPen(Qt::NoPen); p.setBrush(g_t.hover); p.drawRoundedRect(QRectF(rect()).adjusted(0, 1, 0, -1), 10, 10); }
        int lv = 0; MC()->statusOf(m_e, &lv);
        p.setPen(Qt::NoPen); p.setBrush(lv == 1 ? g_t.ok : (lv == 2 ? g_t.danger : g_t.text2)); p.drawEllipse(QPointF(16, height() / 2.0), 4, 4);
        int right = width() - 12; if (m_active) { p.drawPixmap(right - 18, (height() - 18) / 2, iconPix("check", 18, g_t.accent)); right -= 26; }
        QFont f = font(); f.setPixelSize(14); f.setWeight(QFont::Medium); p.setFont(f); p.setPen(g_t.text);
        const int x = 32, avail = qMax(20, right - x);
        p.drawText(QRect(x, 7, avail, 20), Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(f).elidedText(m_e.name, Qt::ElideRight, avail));
        QStringList parts; parts << m_e.family; if (m_e.paramsB > 0) parts << fmtParams(m_e.paramsB); if (!m_e.quant.isEmpty()) parts << m_e.quant; if (m_e.ctx > 0) parts << QString::number(m_e.ctx) + " bağlam";
        QFont sf = font(); sf.setPixelSize(12); p.setFont(sf); p.setPen(g_t.text2);
        p.drawText(QRect(x, 28, avail, 18), Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(sf).elidedText(parts.join(" · "), Qt::ElideRight, avail));
    }
    ModelEntry m_e; bool m_active;
};

class ModelPopup : public PopupPanel {
    Q_OBJECT
public:
    explicit ModelPopup(QWidget *parent) : PopupPanel(parent) {
        auto *top = new QWidget(this); auto *th = hb(top, 0, 0);
        auto *sc = new QScrollArea(top); sc->setWidgetResizable(true); sc->setFixedSize(340, 400); sc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *host = new QWidget; m_ll = vb(host, 4, 2); sc->setWidget(host);
        auto *div = new QFrame(top); div->setFixedWidth(1); div->setStyleSheet(QStringLiteral("background:%1;").arg(g_t.border.name()));
        auto *right = new QWidget(top); right->setFixedWidth(380); auto *rv = vb(right, 14, 10);
        m_detail = new QLabel(right); m_detail->setWordWrap(true); m_detail->setTextFormat(Qt::RichText); m_detail->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_detail->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        rv->addWidget(m_detail); rv->addStretch(1);
        rv->addWidget(mkLabel(QStringLiteral("Anlık bellek"), "small", right)); rv->addWidget(new HardwarePanel(true, right));
        th->addWidget(sc); th->addWidget(div); th->addWidget(right);
        auto *foot = new QWidget(this); auto *fh = hb(foot, 6, 6);
        auto *add = fb("plus", QStringLiteral("Özel model ekle…"), foot)->isz(18)->fixed(170, 34);
        auto *mng = fb("settings", QStringLiteral("Model yönetimi"), foot)->isz(18)->fixed(160, 34);
        fh->addWidget(add); fh->addWidget(mng); fh->addStretch(1);
        body()->addWidget(top); body()->addWidget(foot);
        connect(add, &QAbstractButton::clicked, this, [this] { close(); emit addRequested(); });
        connect(mng, &QAbstractButton::clicked, this, [this] { close(); emit manageRequested(); });
        rebuild();
    }
signals:
    void addRequested();
    void manageRequested();
private:
    void rebuild() {
        const QList<ModelEntry> all = MC()->all(); const QString act = MC()->activeId();
        const QStringList groups = {QStringLiteral("Yerel"), QStringLiteral("Bulut"), QStringLiteral("API")};
        const QStringList titles = {QStringLiteral("Yerel modeller"), QStringLiteral("Bulut modelleri"), QStringLiteral("API modelleri (uyumlu / özel)")};
        for (int g = 0; g < groups.size(); ++g) {
            bool any = false;
            for (const ModelEntry &e : all) {
                if (e.location() != groups[g]) continue;
                if (!any) { auto *hd = mkLabel(titles[g], "small"); hd->setContentsMargins(10, 8, 0, 2); m_ll->addWidget(hd); any = true; }
                auto *r = new ModelRow(e, e.id == act); m_ll->addWidget(r);
                connect(r, &ModelRow::hovered, this, [this](const ModelEntry &en) { m_detail->setText(modelDetailHtml(en)); });
                connect(r, &QAbstractButton::clicked, this, [this, e] { MC()->setActive(e.id); close(); });
            }
        }
        m_ll->addStretch(1);
        m_detail->setText(modelDetailHtml(MC()->active()));
    }
    QVBoxLayout *m_ll; QLabel *m_detail;
};

class ModelSelector : public FlatButton {
    Q_OBJECT
public:
    explicit ModelSelector(QWidget *p = nullptr) : FlatButton(QString(), QString(), p) {
        strong(); trail("chevdown"); setMaximumWidth(300);
        QFont f = font(); f.setWeight(QFont::DemiBold); setFont(f);
        connect(MC(), &ModelCatalog::activeChanged, this, [this] { refreshText(); });
        connect(MC(), &ModelCatalog::changed, this, [this] { refreshText(); });
        connect(this, &QAbstractButton::clicked, this, [this] {
            auto *pop = new ModelPopup(window()); pop->setAttribute(Qt::WA_DeleteOnClose);
            connect(pop, &ModelPopup::addRequested, this, &ModelSelector::addRequested);
            connect(pop, &ModelPopup::manageRequested, this, &ModelSelector::manageRequested);
            pop->showNear(this, false);
        });
        refreshText();
    }
signals:
    void addRequested();
    void manageRequested();
private:
    void refreshText() { setText(MC()->active().name); updateGeometry(); }
};

class ModelEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ModelEditDialog(const ModelEntry &init, bool isNew, QWidget *parent = nullptr) : QDialog(parent), m_e(init) {
        setWindowTitle(isNew ? QStringLiteral("Özel model ekle") : QStringLiteral("Modeli düzenle")); setModal(true); setMinimumWidth(560);
        auto *v = vb(this, 22, 12);
        v->addWidget(mkLabel(isNew ? QStringLiteral("Özel model ekle") : QStringLiteral("Modeli düzenle"), "h2"));
        auto *info = mkLabel(QStringLiteral("Bellek tahmini için gerçek model bilgilerinizi girin. Bilmediğiniz alanları boş bırakın; bilinmeyen değerler “Bilinmiyor” olarak gösterilir, uydurulmaz."), "small"); info->setWordWrap(true); v->addWidget(info);
        auto *g = new QGridLayout; g->setHorizontalSpacing(12); g->setVerticalSpacing(10); int r = 0;
        auto add = [&](const QString &lbl, QWidget *w) { g->addWidget(mkLabel(lbl, "muted"), r, 0); g->addWidget(w, r, 1); ++r; };
        m_name = new QLineEdit(init.name); add(QStringLiteral("Model adı"), m_name);
        m_prov = new QComboBox; m_prov->addItem(pkName(PK::Local), "local");
        for (const ProviderCfg &c : AM()->providers()) m_prov->addItem(c.name + " · " + pkName(c.kind), c.id);
        { int ix = m_prov->findData(init.provId.isEmpty() ? QStringLiteral("local") : init.provId); m_prov->setCurrentIndex(ix < 0 ? 0 : ix); }
        add(QStringLiteral("Sağlayıcı / uç nokta"), m_prov);
        m_api = new QLineEdit(init.apiId); m_api->setPlaceholderText(QStringLiteral("Boş: sağlayıcının seçili modeli kullanılır")); add(QStringLiteral("API model kimliği"), m_api);
        m_params = new QLineEdit(init.paramsB > 0 ? QString::number(init.paramsB) : QString()); m_params->setPlaceholderText(QStringLiteral("örn. 7 (milyar)")); add(QStringLiteral("Parametre (milyar)"), m_params);
        m_quant = new QComboBox; m_quant->setEditable(true); m_quant->addItems({"", "F16", "BF16", "Q8_0", "Q6_K", "Q5_K_M", "Q4_K_M", "Q4", "Q3_K_M", "INT4", "FP8"}); m_quant->setCurrentText(init.quant); add(QStringLiteral("Kuantizasyon"), m_quant);
        m_ctx = new QLineEdit(init.ctx > 0 ? QString::number(init.ctx) : QString()); m_ctx->setPlaceholderText(QStringLiteral("örn. 8192")); add(QStringLiteral("Bağlam uzunluğu"), m_ctx);
        m_layers = new QLineEdit(init.layers > 0 ? QString::number(init.layers) : QString()); add(QStringLiteral("Katman sayısı (isteğe bağlı)"), m_layers);
        m_kvh = new QLineEdit(init.kvHeads > 0 ? QString::number(init.kvHeads) : QString()); add(QStringLiteral("KV head sayısı (isteğe bağlı)"), m_kvh);
        m_hd = new QLineEdit(init.headDim > 0 ? QString::number(init.headDim) : QString()); add(QStringLiteral("Head boyutu (isteğe bağlı)"), m_hd);
        m_vis = new QComboBox; m_vis->addItems({QStringLiteral("Otomatik / bilinmiyor"), QStringLiteral("Evet"), QStringLiteral("Hayır")}); m_vis->setCurrentIndex(init.vision < 0 ? 0 : (init.vision == 1 ? 1 : 2)); add(QStringLiteral("Görsel desteği"), m_vis);
        v->addLayout(g);
        m_est = new QLabel; m_est->setWordWrap(true); m_est->setProperty("role", "muted"); v->addWidget(m_est);
        m_err = mkLabel(QString(), "err"); m_err->setWordWrap(true); v->addWidget(m_err);
        auto *bh = new QHBoxLayout; bh->addStretch(1);
        auto *cancel = new QPushButton(QStringLiteral("İptal")); auto *ok = new QPushButton(QStringLiteral("Kaydet")); ok->setProperty("kind", "primary"); ok->setDefault(true);
        bh->addWidget(cancel); bh->addWidget(ok); v->addLayout(bh);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(ok, &QPushButton::clicked, this, [this] { if (collect()) accept(); });
        for (QLineEdit *le : {m_params, m_ctx, m_layers, m_kvh, m_hd}) connect(le, &QLineEdit::textChanged, this, [this] { updEst(); });
        connect(m_quant, &QComboBox::currentTextChanged, this, [this] { updEst(); });
        connect(m_prov, &QComboBox::currentIndexChanged, this, [this] { updEst(); });
        updEst();
    }
    ModelEntry result() const { return m_e; }
private:
    bool parse(const QString &t, double *out, const QString &what, bool intOnly = false) {
        if (t.trimmed().isEmpty()) { *out = -1; return true; }
        bool ok = false; const double d = QLocale::c().toDouble(QString(t).replace(',', '.'), &ok);
        if (!ok || d <= 0 || (intOnly && d != std::floor(d))) { m_err->setText(what + QStringLiteral(" için geçerli pozitif bir sayı girin.")); return false; }
        *out = d; return true;
    }
    ModelEntry build(bool strict) {
        ModelEntry e = m_e; e.name = m_name->text().trimmed(); e.provId = m_prov->currentData().toString(); e.apiId = m_api->text().trimmed(); e.quant = m_quant->currentText().trimmed();
        double a = -1, b = -1, c = -1, d = -1, f = -1; m_err->clear();
        if (!parse(m_params->text(), &a, QStringLiteral("Parametre")) && strict) return ModelEntry();
        if (!parse(m_ctx->text(), &b, QStringLiteral("Bağlam uzunluğu"), true) && strict) return ModelEntry();
        if (!parse(m_layers->text(), &c, QStringLiteral("Katman sayısı"), true) && strict) return ModelEntry();
        if (!parse(m_kvh->text(), &d, QStringLiteral("KV head sayısı"), true) && strict) return ModelEntry();
        if (!parse(m_hd->text(), &f, QStringLiteral("Head boyutu"), true) && strict) return ModelEntry();
        e.paramsB = a; e.ctx = b > 0 ? int(b) : 0; e.layers = c > 0 ? int(c) : 0; e.kvHeads = d > 0 ? int(d) : 0; e.headDim = f > 0 ? int(f) : 0;
        e.vision = m_vis->currentIndex() == 0 ? -1 : (m_vis->currentIndex() == 1 ? 1 : 0);
        if (e.provId == "local") e.kind = PK::Local; else { bool ok = false; const ProviderCfg c2 = AM()->find(e.provId, &ok); e.kind = ok ? c2.kind : PK::Custom; }
        return e;
    }
    void updEst() { const ModelEntry e = build(false); m_est->setText(QStringLiteral("Bellek tahmini: ") + memSummary(e)); }
    bool collect() {
        const ModelEntry e = build(true);
        if (!m_err->text().isEmpty()) return false;
        if (e.name.isEmpty()) { m_err->setText(QStringLiteral("Model adı zorunludur.")); return false; }
        m_e = e; return true;
    }
    ModelEntry m_e; QLineEdit *m_name, *m_api, *m_params, *m_ctx, *m_layers, *m_kvh, *m_hd; QComboBox *m_prov, *m_quant, *m_vis; QLabel *m_est, *m_err;
};

// ============================================================================
// 7) Mesaj giriş alanı (composer)
// ============================================================================
static QString attachDir() {
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/attachments");
    QDir().mkpath(d); return d;
}
static bool isImageFile(const QString &p) {
    const QString e = QFileInfo(p).suffix().toLower();
    return e == "png" || e == "jpg" || e == "jpeg" || e == "webp" || e == "bmp";
}

class ComposerEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit ComposerEdit(QWidget *p = nullptr) : QTextEdit(p) {
        setObjectName("composerEdit"); setAcceptRichText(false); setFrameShape(QFrame::NoFrame); document()->setDocumentMargin(0);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setPlaceholderText(QStringLiteral("Mesajınızı yazın…"));
    }
signals:
    void sendRequested();
    void imagesDropped(const QStringList &paths);
protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            const bool ctrlMode = !Cfg::get("enterSends", true).toBool();
            const bool ctrl = e->modifiers() & Qt::ControlModifier, shift = e->modifiers() & Qt::ShiftModifier;
            if (ctrlMode ? ctrl : (!shift && !ctrl)) { emit sendRequested(); return; }
            if (ctrl) { insertPlainText("\n"); return; }
        }
        QTextEdit::keyPressEvent(e);
    }
    bool canInsertFromMimeData(const QMimeData *s) const override { return s->hasImage() || s->hasUrls() || s->hasText(); }
    void insertFromMimeData(const QMimeData *s) override {
        if (s->hasImage()) {
            const QImage img = qvariant_cast<QImage>(s->imageData());
            if (!img.isNull()) { const QString p = attachDir() + "/paste_" + newId() + ".png"; if (img.save(p, "PNG")) { emit imagesDropped(QStringList{p}); return; } }
        }
        if (s->hasUrls()) {
            QStringList imgs; for (const QUrl &u : s->urls()) if (u.isLocalFile() && isImageFile(u.toLocalFile())) imgs << u.toLocalFile();
            if (!imgs.isEmpty()) { emit imagesDropped(imgs); return; }
        }
        if (s->hasText()) insertPlainText(s->text());
    }
};

class AttachThumb : public QWidget {
    Q_OBJECT
public:
    explicit AttachThumb(const QString &path, QWidget *p = nullptr) : QWidget(p), m_path(path) {
        setFixedSize(76, 72); setCursor(Qt::PointingHandCursor); m_pm = thumbPix(path, 64);
        setToolTip(QFileInfo(path).fileName());
    }
    const QString &path() const { return m_path; }
signals:
    void removeRequested(const QString &path);
protected:
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton) return;
        if (QLineF(e->position(), QPointF(66, 10)).length() <= 12) emit removeRequested(m_path);
        else QDesktopServices::openUrl(QUrl::fromLocalFile(m_path));
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QRect ir(2, 8, 64, 64); QPainterPath pp; pp.addRoundedRect(QRectF(ir), 12, 12); p.save(); p.setClipPath(pp);
        if (m_pm.isNull()) { p.fillRect(ir, g_t.hover); p.setPen(g_t.text2); QFont f = font(); f.setPixelSize(11); p.setFont(f); p.drawText(ir, Qt::AlignCenter, QFileInfo(m_path).suffix().toUpper()); }
        else p.drawPixmap(ir, m_pm);
        p.restore(); p.setBrush(Qt::NoBrush); p.setPen(QPen(g_t.border, 1)); p.drawRoundedRect(QRectF(ir).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
        p.setPen(Qt::NoPen); p.setBrush(g_t.text); p.drawEllipse(QPointF(66, 10), 9, 9);
        p.drawPixmap(QPointF(60, 4), iconPix("x", 12, g_t.bg));
    }
    QString m_path; QPixmap m_pm;
};

class SendButton : public QAbstractButton {
    Q_OBJECT
public:
    enum Mode { Idle, Ready, Busy };
    explicit SendButton(QWidget *p = nullptr) : QAbstractButton(p) { setFixedSize(36, 36); setFocusPolicy(Qt::NoFocus); setMode(Idle); connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); }); }
    void setMode(Mode m) { m_mode = m; setCursor(m == Idle ? Qt::ArrowCursor : Qt::PointingHandCursor); setToolTip(m == Busy ? QStringLiteral("Durdur") : QStringLiteral("Gönder")); update(); }
    Mode mode() const { return m_mode; }
protected:
    void enterEvent(QEnterEvent *e) override { QAbstractButton::enterEvent(e); update(); }
    void leaveEvent(QEvent *e) override { QAbstractButton::leaveEvent(e); update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QColor bg, fg = Qt::white;
        if (m_mode == Busy) { bg = g_t.text; fg = g_t.bg; if (underMouse()) bg = blendC(bg, g_t.text2, 0.25); }
        else if (m_mode == Ready) bg = underMouse() ? g_t.accentHi : g_t.accent;
        else { bg = g_t.dark ? C("#4A4A4A") : C("#D5D5D5"); fg = g_t.dark ? C("#8A8A8A") : Qt::white; }
        p.setPen(Qt::NoPen); p.setBrush(bg); p.drawEllipse(rect().adjusted(1, 1, -1, -1));
        p.drawPixmap((width() - 20) / 2, (height() - 20) / 2, iconPix(m_mode == Busy ? "stop" : "arrowup", 20, fg));
    }
    Mode m_mode = Idle;
};

class MessageComposer : public QWidget {
    Q_OBJECT
public:
    static constexpr int M = 14;
    explicit MessageComposer(QWidget *p = nullptr) : QWidget(p) {
        m_think = qBound(0, Cfg::get("think", 0).toInt(), 3);
        auto *root = vb(this); root->setContentsMargins(M + 14, M + 12, M + 14, M + 8); root->setSpacing(4);
        m_strip = new QWidget(this); m_stripLay = hb(m_strip, 0, 8); m_stripLay->setContentsMargins(0, 0, 0, 4); m_stripLay->addStretch(1); m_strip->hide();
        m_edit = new ComposerEdit(this); m_edit->setFixedHeight(28);
        auto *row = new QHBoxLayout; row->setSpacing(6); row->setContentsMargins(0, 0, 0, 0);
        m_attach = fb("plus", QString(), this)->isz(22)->rad(-1)->fixed(36, 36)->tip(QStringLiteral("Görsel ekle"));
        m_thinkBtn = fb("brain", QString(), this)->isz(18)->fixed(120, 34)->rad(17);
        m_mic = fb("mic", QString(), this)->isz(20)->rad(-1)->fixed(36, 36)->tip(QStringLiteral("Sesle yazma (bu sürümde yok)"));
        m_send = new SendButton(this);
        row->addWidget(m_attach); row->addStretch(1); row->addWidget(m_thinkBtn); row->addWidget(m_mic); row->addWidget(m_send);
        root->addWidget(m_strip); root->addWidget(m_edit); root->addLayout(row);
        m_hAnim = new QVariantAnimation(this); m_hAnim->setDuration(110); m_hAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_hAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { m_edit->setFixedHeight(v.toInt()); });
        connect(m_edit, &QTextEdit::textChanged, this, [this] { adjustEdit(); updateSend(); });
        connect(m_edit, &ComposerEdit::sendRequested, this, [this] { if (!m_gen) trySend(); });
        connect(m_edit, &ComposerEdit::imagesDropped, this, [this](const QStringList &l) { addImages(l); });
        connect(m_send, &QAbstractButton::clicked, this, [this] { if (m_gen) emit stopRequested(); else trySend(); });
        connect(m_attach, &QAbstractButton::clicked, this, [this] {
            const QStringList f = QFileDialog::getOpenFileNames(window(), QStringLiteral("Görsel seç"), Cfg::get("lastImgDir", QDir::homePath()).toString(), QStringLiteral("Görseller (*.png *.jpg *.jpeg *.webp *.bmp)"));
            if (!f.isEmpty()) { Cfg::set("lastImgDir", QFileInfo(f.first()).absolutePath()); addImages(f); }
        });
        connect(m_mic, &QAbstractButton::clicked, this, [this] { toast(this, QStringLiteral("Ses tanıma bu sürümde kullanılamıyor (Qt Speech modülü eklenmedi). Düğme yalnızca arayüz öğesidir."), true); });
        connect(m_thinkBtn, &QAbstractButton::clicked, this, [this] { openThinking(); });
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
        refreshThink(); updateSend();
    }
    QString text() const { return m_edit->toPlainText().trimmed(); }
    QStringList images() const { return m_images; }
    int thinking() const { return m_think; }
    bool generating() const { return m_gen; }
    void focusInput() { m_edit->setFocus(); }
    void setThinkingSupport(bool ok) { m_thinkOk = ok; refreshThink(); }
    void setGenerating(bool g) { m_gen = g; updateSend(); }
    void clearInput() { m_edit->clear(); m_images.clear(); rebuildStrip(); updateSend(); adjustEdit(); }
    void setText(const QString &t) { m_edit->setPlainText(t); }
    void addImages(const QStringList &paths) {
        for (const QString &src : paths) {
            if (m_images.size() >= 8) { toast(this, QStringLiteral("En fazla 8 görsel ekleyebilirsiniz."), true); break; }
            const QFileInfo fi(src);
            if (!fi.exists() || !fi.isReadable()) { toast(this, QStringLiteral("Görsel eklenemedi: dosya bulunamadı veya okunamıyor (%1).").arg(fi.fileName()), true); continue; }
            if (!isImageFile(src)) { toast(this, QStringLiteral("Desteklenmeyen biçim: %1 (PNG, JPG, JPEG, WEBP, BMP).").arg(fi.fileName()), true); continue; }
            if (fi.size() > 20LL * 1024 * 1024) { toast(this, QStringLiteral("Görsel çok büyük (en fazla 20 MB): %1").arg(fi.fileName()), true); continue; }
            QString dst = src;
            if (!QDir::cleanPath(fi.absolutePath()).startsWith(QDir::cleanPath(attachDir()))) {
                dst = attachDir() + "/" + newId() + "." + fi.suffix().toLower();
                if (!QFile::copy(src, dst)) { toast(this, QStringLiteral("Görsel yüklenemedi (kopyalama hatası): %1").arg(fi.fileName()), true); continue; }
            }
            if (!m_images.contains(dst)) m_images << dst;
        }
        rebuildStrip(); updateSend();
    }
signals:
    void sendRequested(const QString &text, const QStringList &images);
    void stopRequested();
    void heightChanged();
    void thinkingChanged(int mode);
protected:
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e); adjustEdit(); emit heightChanged(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(rect()).adjusted(M, M, -M, -M); p.setPen(Qt::NoPen);
        for (int i = 6; i >= 1; --i) { p.setBrush(QColor(0, 0, 0, g_t.dark ? 20 : 9)); p.drawRoundedRect(r.adjusted(-i * 1.4, -i * 1.4 + 3, i * 1.4, i * 1.4 + 3), 26 + i, 26 + i); }
        p.setBrush(g_t.composer); p.setPen(QPen(g_t.border, 1)); p.drawRoundedRect(r, 26, 26);
    }
private:
    void trySend() {
        const QString t = text(); if (t.isEmpty() && m_images.isEmpty()) return;
        emit sendRequested(t, m_images);
    }
    void updateSend() { m_send->setMode(m_gen ? SendButton::Busy : ((!text().isEmpty() || !m_images.isEmpty()) ? SendButton::Ready : SendButton::Idle)); }
    void adjustEdit() {
        QTextDocument *d = m_edit->document(); d->setTextWidth(qMax(50, m_edit->viewport()->width()));
        const int h = qBound(28, int(d->documentLayout()->documentSize().height()) + 4, 220);
        m_edit->setVerticalScrollBarPolicy(h >= 220 ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
        if (h == m_edit->height() && m_hAnim->state() != QAbstractAnimation::Running) return;
        if (Cfg::anim() && isVisible()) { m_hAnim->stop(); m_hAnim->setStartValue(m_edit->height()); m_hAnim->setEndValue(h); m_hAnim->start(); }
        else m_edit->setFixedHeight(h);
    }
    void rebuildStrip() {
        while (m_stripLay->count() > 1) { QLayoutItem *it = m_stripLay->takeAt(0); if (it->widget()) it->widget()->deleteLater(); delete it; }
        for (const QString &p : m_images) {
            auto *t = new AttachThumb(p, m_strip);
            connect(t, &AttachThumb::removeRequested, this, [this](const QString &path) {
                m_images.removeAll(path);
                if (QDir::cleanPath(QFileInfo(path).absolutePath()).startsWith(QDir::cleanPath(attachDir()))) QFile::remove(path);
                rebuildStrip(); updateSend();
            });
            m_stripLay->insertWidget(m_stripLay->count() - 1, t);
        }
        m_strip->setVisible(!m_images.isEmpty());
    }
    void refreshThink() {
        static const char *names[4] = {"Hızlı", "Dengeli", "Yüksek", "Maksimum"};
        m_thinkBtn->setText(QString::fromUtf8(names[m_think]));
        m_thinkBtn->setToolTip(m_thinkOk ? QStringLiteral("Düşünme modu") : QStringLiteral("Düşünme modu — seçili model/sağlayıcı bu ayarı desteklemiyor; istekte kullanılmaz"));
        m_thinkBtn->tint(m_thinkOk ? QColor() : g_t.text2);
    }
    void openThinking() {
        auto *pop = new PopupPanel(window()); pop->setAttribute(Qt::WA_DeleteOnClose);
        auto *hd = mkLabel(QStringLiteral("Düşünme modu"), "small"); hd->setContentsMargins(12, 6, 0, 4); pop->body()->addWidget(hd);
        static const char *names[4] = {"Hızlı", "Dengeli", "Yüksek", "Maksimum"};
        static const char *desc[4] = {"Ek akıl yürütme istenmez", "Orta düzey akıl yürütme", "Derin akıl yürütme", "En yüksek akıl yürütme bütçesi"};
        for (int i = 0; i < 4; ++i) {
            auto *r = new MenuRow("brain", QString::fromUtf8(names[i]), QString::fromUtf8(desc[i])); r->check(i == m_think); r->setMinimumWidth(300);
            connect(r, &QAbstractButton::clicked, this, [this, pop, i] { m_think = i; Cfg::set("think", i); refreshThink(); emit thinkingChanged(i); pop->close(); });
            pop->body()->addWidget(r);
        }
        if (!m_thinkOk) {
            auto *n = mkLabel(QStringLiteral("Seçili model/sağlayıcı bu ayarı desteklemiyor; seçim istekte kullanılmaz."), "small"); n->setWordWrap(true); n->setContentsMargins(12, 6, 12, 6); n->setMaximumWidth(300); pop->body()->addWidget(n);
        }
        pop->showNear(m_thinkBtn, true, true);
    }
    ComposerEdit *m_edit; QWidget *m_strip; QHBoxLayout *m_stripLay; FlatButton *m_attach, *m_thinkBtn, *m_mic; SendButton *m_send; QVariantAnimation *m_hAnim;
    QStringList m_images; int m_think = 0; bool m_gen = false, m_thinkOk = false;
};

// ============================================================================
// 8) Kenar çubuğu ve sohbet başlığı
// ============================================================================
static bool confirmBox(QWidget *parent, const QString &title, const QString &text, const QString &yes = QStringLiteral("Sil")) {
    QMessageBox b(parent); b.setIcon(QMessageBox::Warning); b.setWindowTitle(title); b.setText(text);
    QPushButton *y = b.addButton(yes, QMessageBox::AcceptRole); b.addButton(QStringLiteral("İptal"), QMessageBox::RejectRole); b.setDefaultButton(y);
    b.exec(); return b.clickedButton() == y;
}
static bool askText(QWidget *parent, const QString &title, const QString &label, QString *value) {
    QInputDialog d(parent); d.setWindowTitle(title); d.setLabelText(label); d.setTextValue(*value); d.setOkButtonText(QStringLiteral("Tamam")); d.setCancelButtonText(QStringLiteral("İptal"));
    d.setMinimumWidth(380);
    if (d.exec() != QDialog::Accepted) return false;
    *value = d.textValue().trimmed(); return true;
}
static QStringList projectList() { return Cfg::get("projects").toStringList(); }
static void setProjectList(const QStringList &l) { Cfg::set("projects", l); }

enum { RKind = Qt::UserRole + 1, RId, RIcon, RExtra, RActive };
enum ItemKind { KNav = 0, KHeader, KProject, KConv, KEmpty, KSpacer };

class SideList;
class SideDelegate : public QStyledItemDelegate {
public:
    SideDelegate(SideList *l, QObject *parent) : QStyledItemDelegate(parent), m_list(l) {}
    QSize sizeHint(const QStyleOptionViewItem &o, const QModelIndex &i) const override {
        switch (i.data(RKind).toInt()) {
        case KNav: return QSize(o.rect.width(), 40);
        case KHeader: return QSize(o.rect.width(), 36);
        case KProject: return QSize(o.rect.width(), 38);
        case KConv: return QSize(o.rect.width(), 40);
        case KEmpty: return QSize(o.rect.width(), 34);
        default: return QSize(o.rect.width(), 12);
        }
    }
    void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override;
private:
    SideList *m_list;
};

class SideList : public QListWidget {
    Q_OBJECT
public:
    explicit SideList(QWidget *p = nullptr) : QListWidget(p) {
        setSelectionMode(QAbstractItemView::NoSelection); setMouseTracking(true); setFrameShape(QFrame::NoFrame); setFocusPolicy(Qt::NoFocus);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setItemDelegate(new SideDelegate(this, this));
        verticalScrollBar()->setSingleStep(20);
    }
    int hoverRow() const { return m_hover; }
signals:
    void activatedItem(QListWidgetItem *it);
    void convMenu(const QString &id, const QPoint &globalPos);
    void projectMenu(const QString &name, const QPoint &globalPos);
    void addProject();
protected:
    void mouseMoveEvent(QMouseEvent *e) override { const int r = row(itemAt(e->position().toPoint())); if (r != m_hover) { m_hover = r; viewport()->update(); } QListWidget::mouseMoveEvent(e); }
    void leaveEvent(QEvent *e) override { m_hover = -1; viewport()->update(); QListWidget::leaveEvent(e); }
    void mouseReleaseEvent(QMouseEvent *e) override {
        QListWidgetItem *it = itemAt(e->position().toPoint());
        if (!it || e->button() != Qt::LeftButton) return;
        const int kind = it->data(RKind).toInt(); const QRect r = visualItemRect(it); const bool onRight = e->position().x() > r.right() - 42;
        if (kind == KConv && onRight) { emit convMenu(it->data(RId).toString(), e->globalPosition().toPoint()); return; }
        if (kind == KProject && onRight) { emit projectMenu(it->data(RId).toString(), e->globalPosition().toPoint()); return; }
        if (kind == KHeader) { if (it->data(RExtra).toBool() && onRight) emit addProject(); return; }
        if (kind == KEmpty || kind == KSpacer) return;
        emit activatedItem(it);
    }
    void contextMenuEvent(QContextMenuEvent *e) override {
        QListWidgetItem *it = itemAt(e->pos()); if (!it) return;
        const int kind = it->data(RKind).toInt();
        if (kind == KConv) emit convMenu(it->data(RId).toString(), e->globalPos());
        else if (kind == KProject) emit projectMenu(it->data(RId).toString(), e->globalPos());
    }
    int m_hover = -1;
};

void SideDelegate::paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const {
    const int kind = i.data(RKind).toInt(); if (kind == KSpacer) return;
    p->save(); p->setRenderHint(QPainter::Antialiasing);
    const bool hov = m_list->hoverRow() == i.row(), act = i.data(RActive).toBool();
    const QRect r = o.rect.adjusted(6, 1, -6, -1);
    const QString text = i.data(Qt::DisplayRole).toString();
    QFont f = o.font; f.setPixelSize(14);
    if (kind == KHeader) {
        f.setPixelSize(13); f.setWeight(QFont::DemiBold); p->setFont(f); p->setPen(g_t.text2);
        p->drawText(QRect(r.left() + 10, r.top() + 6, r.width() - 50, r.height() - 6), Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(f).elidedText(text, Qt::ElideRight, r.width() - 50));
        if (i.data(RExtra).toBool()) p->drawPixmap(r.right() - 28, r.top() + 12, iconPix("plus", 16, hov ? g_t.text : g_t.text2));
        p->restore(); return;
    }
    if (kind == KEmpty) {
        f.setPixelSize(13); p->setFont(f); QColor c = g_t.text2; c.setAlphaF(0.8); p->setPen(c); p->drawText(r.adjusted(10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
        p->restore(); return;
    }
    if (hov || act) { p->setPen(Qt::NoPen); p->setBrush(act ? g_t.sel : g_t.hover); p->drawRoundedRect(r, 9, 9); }
    int x = r.left() + 12;
    const QString icon = i.data(RIcon).toString();
    if (!icon.isEmpty()) { p->drawPixmap(x, r.top() + (r.height() - 20) / 2, iconPix(icon, 20, kind == KProject ? g_t.text2 : g_t.text)); x += 32; }
    int right = r.right() - 8;
    if (kind == KConv || kind == KProject) {
        if (hov || act) { p->drawPixmap(right - 20, r.top() + (r.height() - 20) / 2, iconPix("more", 20, g_t.text2)); right -= 28; }
        else if (kind == KConv && i.data(RExtra).toBool()) { p->drawPixmap(right - 14, r.top() + (r.height() - 14) / 2, iconPix("pin", 14, g_t.text2)); right -= 22; }
    }
    p->setFont(f); p->setPen(g_t.text);
    p->drawText(QRect(x, r.top(), qMax(10, right - x), r.height()), Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(f).elidedText(text, Qt::ElideRight, qMax(10, right - x)));
    p->restore();
}

class ProfileButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit ProfileButton(QWidget *p = nullptr) : QAbstractButton(p) {
        setCursor(Qt::PointingHandCursor); setFixedHeight(56); setFocusPolicy(Qt::NoFocus);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
    void setUser(const QString &ini, const QString &name, const QString &type) { m_ini = ini; m_name = name; m_type = type; update(); }
protected:
    void enterEvent(QEnterEvent *e) override { QAbstractButton::enterEvent(e); update(); }
    void leaveEvent(QEvent *e) override { QAbstractButton::leaveEvent(e); update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        if (underMouse()) { p.setPen(Qt::NoPen); p.setBrush(g_t.hover); p.drawRoundedRect(QRectF(rect()).adjusted(6, 4, -6, -4), 10, 10); }
        p.setPen(Qt::NoPen); p.setBrush(C("#8A93A3")); p.drawEllipse(QPointF(30, height() / 2.0), 16, 16);
        QFont f = font(); f.setPixelSize(13); f.setWeight(QFont::DemiBold); p.setFont(f); p.setPen(Qt::white); p.drawText(QRectF(14, height() / 2.0 - 16, 32, 32), Qt::AlignCenter, m_ini);
        QFont nf = font(); nf.setPixelSize(14); nf.setWeight(QFont::DemiBold); p.setFont(nf); p.setPen(g_t.text);
        const int aw = width() - 54 - 40;
        p.drawText(QRect(54, 9, aw, 20), Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(nf).elidedText(m_name, Qt::ElideRight, aw));
        QFont tf = font(); tf.setPixelSize(12); p.setFont(tf); p.setPen(g_t.text2); p.drawText(QRect(54, 29, aw, 18), Qt::AlignVCenter | Qt::AlignLeft, m_type);
        p.drawPixmap(width() - 34, (height() - 16) / 2, iconPix("chevup", 16, g_t.text2));
    }
    QString m_ini, m_name, m_type;
};

class Sidebar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int panelWidth READ panelWidth WRITE setPanelWidth)
public:
    explicit Sidebar(QWidget *p = nullptr) : QWidget(p) {
        m_full = 340; m_w = Cfg::get("sidebarOpen", true).toBool() ? m_full : 0; m_open = m_w > 0;
        m_inner = new QWidget(this);
        auto *v = vb(m_inner);
        auto *top = new QWidget(m_inner); top->setFixedHeight(58); auto *th = hb(top, 0, 4); th->setContentsMargins(18, 0, 12, 0);
        auto *logo = new LogoLabel(26, top); auto *brand = mkLabel(QStringLiteral("RÜZGAR AI"), "brand", top);
        m_searchBtn = fb("search", QString(), top)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Sohbetlerde ara (Ctrl+K)"));
        auto *col = fb("sidebar", QString(), top)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Kenar çubuğunu kapat (Ctrl+B)"));
        th->addWidget(logo); th->addSpacing(6); th->addWidget(brand); th->addStretch(1); th->addWidget(m_searchBtn); th->addWidget(col);
        m_search = new QLineEdit(m_inner); m_search->setPlaceholderText(QStringLiteral("Sohbetlerde ara…")); m_search->setClearButtonEnabled(true); m_search->hide();
        auto *sw = new QWidget(m_inner); auto *sl = hb(sw, 0); sl->setContentsMargins(14, 0, 14, 8); sl->addWidget(m_search); m_searchWrap = sw; sw->hide();
        m_list = new SideList(m_inner);
        auto *line = new QFrame(m_inner); line->setFixedHeight(1); line->setObjectName("sbLine");
        m_profile = new ProfileButton(m_inner);
        v->addWidget(top); v->addWidget(sw); v->addWidget(m_list, 1); v->addWidget(line); v->addWidget(m_profile);
        m_anim = new QPropertyAnimation(this, "panelWidth", this); m_anim->setDuration(230); m_anim->setEasingCurve(QEasingCurve::InOutCubic);
        m_deb = new QTimer(this); m_deb->setSingleShot(true); m_deb->setInterval(180); connect(m_deb, &QTimer::timeout, this, [this] { refresh(); });
        connect(col, &QAbstractButton::clicked, this, [this] { emit collapseRequested(); });
        connect(m_searchBtn, &QAbstractButton::clicked, this, [this] { focusSearch(); });
        connect(m_search, &QLineEdit::textChanged, this, [this] { m_deb->start(); });
        connect(m_search, &QLineEdit::editingFinished, this, [this] { if (m_search->text().trimmed().isEmpty() && m_searchWrap->isVisible()) { m_searchWrap->hide(); m_search->hide(); refresh(); } });
        connect(m_list, &SideList::activatedItem, this, &Sidebar::onActivated);
        connect(m_list, &SideList::convMenu, this, &Sidebar::showConvMenu);
        connect(m_list, &SideList::projectMenu, this, &Sidebar::showProjectMenu);
        connect(m_list, &SideList::addProject, this, [this] { QString n; if (askText(window(), QStringLiteral("Yeni proje"), QStringLiteral("Proje adı:"), &n) && !n.isEmpty()) addProjectName(n); });
        connect(m_profile, &QAbstractButton::clicked, this, [this] { openAccountMenu(); });
        connect(CM(), &ConversationManager::changed, this, [this] { refresh(); });
        connect(UM(), &UserManager::userChanged, this, [this] { updateUser(); refresh(); });
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
        setPanelWidth(m_w); updateUser();
    }
    int panelWidth() const { return m_w; }
    void setPanelWidth(int w) { m_w = w; setMinimumWidth(w); setMaximumWidth(w); m_inner->setVisible(w > 0); layoutInner(); }
    bool isOpen() const { return m_open; }
    void setFullWidth(int w) { m_full = w; if (m_open && m_anim->state() != QAbstractAnimation::Running) setPanelWidth(w); layoutInner(); }
    void setOpen(bool open, bool animate) {
        m_open = open; Cfg::set("sidebarOpen", open);
        const int target = open ? m_full : 0;
        if (animate && Cfg::anim()) { m_anim->stop(); m_anim->setStartValue(m_w); m_anim->setEndValue(target); m_anim->start(); } else setPanelWidth(target);
        emit openChanged(open);
    }
    void setActiveConv(const QString &id) { m_activeConv = id; if (!id.isEmpty()) m_activeSection.clear(); refresh(); }
    void setActiveSection(const QString &s) { m_activeSection = s; if (!s.isEmpty()) m_activeConv.clear(); refresh(); }
    void focusSearch() { setOpen(true, true); m_searchWrap->show(); m_search->show(); m_search->setFocus(); m_search->selectAll(); }
    void updateUser() { if (UM()->loggedIn()) m_profile->setUser(UM()->user().initials, UM()->user().fullName(), QStringLiteral("Yerel hesap")); }
    void refresh() {
        if (!UM()->loggedIn()) return;
        const QString term = m_searchWrap->isVisible() ? m_search->text().trimmed() : QString();
        const int sv = m_list->verticalScrollBar()->value(); m_list->setUpdatesEnabled(false); m_list->clear();
        auto add = [&](int kind, const QString &text, const QString &id = QString(), const QString &icon = QString(), bool extra = false, bool active = false) {
            auto *it = new QListWidgetItem(text); it->setData(RKind, kind); it->setData(RId, id); it->setData(RIcon, icon); it->setData(RExtra, extra); it->setData(RActive, active);
            it->setFlags(Qt::ItemIsEnabled); m_list->addItem(it); return it;
        };
        if (term.isEmpty()) {
            add(KNav, QStringLiteral("Yeni sohbet"), "new", "newchat");
            add(KNav, QStringLiteral("Görseller"), "images", "image", false, m_activeSection == "images");
            add(KNav, QStringLiteral("Görevler"), "tasks", "clock", false, m_activeSection == "tasks");
            add(KNav, QStringLiteral("Kitaplık"), "library", "library", false, m_activeSection == "library");
            add(KNav, QStringLiteral("Keşfet"), "discover", "grid", false, m_activeSection == "discover");
            add(KSpacer, QString()); add(KHeader, QStringLiteral("Projeler"), QString(), QString(), true);
            const QStringList pr = projectList();
            if (pr.isEmpty()) add(KEmpty, QStringLiteral("Proje yok"));
            for (const QString &n : pr) add(KProject, n, n, "folder", false, n == m_project);
            add(KSpacer, QString());
        }
        const QList<ConvInfo> convs = CM()->list(term, term.isEmpty() ? m_project : QString());
        if (!term.isEmpty()) { add(KHeader, QStringLiteral("Arama sonuçları (%1)").arg(convs.size())); }
        else if (!m_project.isEmpty()) add(KHeader, QStringLiteral("Proje: ") + m_project);
        const QDateTime now = QDateTime::currentDateTime(); const qint64 t0 = QDateTime(now.date(), QTime(0, 0)).toMSecsSinceEpoch(); const qint64 day = 86400000LL;
        static const QStringList gl = {QStringLiteral("Sabitlenenler"), QStringLiteral("Bugün"), QStringLiteral("Dün"), QStringLiteral("Önceki 7 gün"), QStringLiteral("Daha eski")};
        QList<ConvInfo> groups[5];
        for (const ConvInfo &c : convs) { int g = c.pinned ? 0 : (c.updated >= t0 ? 1 : (c.updated >= t0 - day ? 2 : (c.updated >= t0 - 7 * day ? 3 : 4))); groups[g].append(c); }
        for (int g = 0; g < 5; ++g) {
            if (groups[g].isEmpty()) continue;
            if (term.isEmpty()) add(KHeader, gl[g]);
            for (const ConvInfo &c : groups[g]) add(KConv, c.title, c.id, QString(), c.pinned, c.id == m_activeConv);
        }
        if (convs.isEmpty()) add(KEmpty, !term.isEmpty() ? QStringLiteral("Sonuç bulunamadı") : QStringLiteral("Henüz sohbet yok"));
        m_list->setUpdatesEnabled(true); m_list->verticalScrollBar()->setValue(sv); m_list->viewport()->update();
    }
signals:
    void newChat();
    void openConv(const QString &id);
    void convDeleted(const QString &id);
    void sectionRequested(const QString &key);
    void accountAction(const QString &key);
    void collapseRequested();
    void openChanged(bool open);
protected:
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e); layoutInner(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.fillRect(rect(), g_t.sidebar);
        p.setPen(g_t.border); if (width() > 1) p.drawLine(width() - 1, 0, width() - 1, height());
    }
private:
    void layoutInner() { m_inner->setGeometry(0, 0, m_full, height()); }
    void onActivated(QListWidgetItem *it) {
        const int kind = it->data(RKind).toInt(); const QString id = it->data(RId).toString();
        if (kind == KNav) { if (id == "new") emit newChat(); else emit sectionRequested(id); }
        else if (kind == KProject) { m_project = (m_project == id) ? QString() : id; refresh(); }
        else if (kind == KConv) emit openConv(id);
    }
    void addProjectName(const QString &n) { QStringList l = projectList(); if (!l.contains(n)) { l << n; setProjectList(l); } refresh(); }
    void showProjectMenu(const QString &name, const QPoint &pos) {
        QMenu m(this);
        QAction *ren = m.addAction(QIcon(iconPix("newchat", 18, g_t.text)), QStringLiteral("Projeyi yeniden adlandır"));
        QAction *del = m.addAction(QIcon(iconPix("trash", 18, g_t.danger)), QStringLiteral("Projeyi sil (sohbetler korunur)"));
        QAction *r = m.exec(pos);
        if (r == ren) {
            QString n = name; if (askText(window(), QStringLiteral("Projeyi yeniden adlandır"), QStringLiteral("Yeni ad:"), &n) && !n.isEmpty() && n != name) {
                QStringList l = projectList(); const int ix = l.indexOf(name); if (ix >= 0 && !l.contains(n)) l[ix] = n; setProjectList(l); CM()->renameProject(name, n); if (m_project == name) m_project = n; refresh();
            }
        } else if (r == del) {
            if (confirmBox(window(), QStringLiteral("Projeyi sil"), QStringLiteral("“%1” projesi silinsin mi? İçindeki sohbetler silinmez.").arg(name))) {
                QStringList l = projectList(); l.removeAll(name); setProjectList(l); CM()->renameProject(name, QString()); if (m_project == name) m_project.clear(); refresh();
            }
        }
    }
    void showConvMenu(const QString &id, const QPoint &pos) {
        const ConvInfo c = CM()->get(id); if (c.id.isEmpty()) return;
        QMenu m(this);
        QAction *aOpen = m.addAction(QIcon(iconPix("chat", 18, g_t.text)), QStringLiteral("Aç"));
        QAction *aRen = m.addAction(QIcon(iconPix("newchat", 18, g_t.text)), QStringLiteral("Yeniden adlandır"));
        QAction *aPin = m.addAction(QIcon(iconPix("pin", 18, g_t.text)), c.pinned ? QStringLiteral("Sabitlemeyi kaldır") : QStringLiteral("Sabitle"));
        QMenu *pm = m.addMenu(QIcon(iconPix("folder", 18, g_t.text)), QStringLiteral("Projeye taşı"));
        for (const QString &n : projectList()) { QAction *a = pm->addAction(n); a->setCheckable(true); a->setChecked(n == c.project); a->setData(QStringList{QStringLiteral("proj"), n}); }
        if (!c.project.isEmpty()) { QAction *a = pm->addAction(QStringLiteral("Projeden çıkar")); a->setData(QStringList{QStringLiteral("proj"), QString()}); }
        QAction *aNew = pm->addAction(QStringLiteral("Yeni proje…"));
        m.addSeparator();
        QAction *aDel = m.addAction(QIcon(iconPix("trash", 18, g_t.danger)), QStringLiteral("Sil"));
        QAction *r = m.exec(pos); if (!r) return;
        if (r == aOpen) emit openConv(id);
        else if (r == aRen) { QString t = c.title; if (askText(window(), QStringLiteral("Sohbeti yeniden adlandır"), QStringLiteral("Yeni ad:"), &t) && !t.isEmpty()) CM()->rename(id, t); }
        else if (r == aPin) CM()->setPinned(id, !c.pinned);
        else if (r == aNew) { QString n; if (askText(window(), QStringLiteral("Yeni proje"), QStringLiteral("Proje adı:"), &n) && !n.isEmpty()) { addProjectName(n); CM()->setProject(id, n); } }
        else if (r == aDel) { if (confirmBox(window(), QStringLiteral("Sohbeti sil"), QStringLiteral("“%1” sohbeti silinsin mi? Bu işlem geri alınamaz.").arg(c.title))) { CM()->remove(id); emit convDeleted(id); } }
        else if (r->data().toStringList().value(0) == "proj") CM()->setProject(id, r->data().toStringList().value(1));
    }
    void openAccountMenu() {
        auto *pop = new PopupPanel(window()); pop->setAttribute(Qt::WA_DeleteOnClose);
        struct E { const char *icon, *text, *key; };
        static const E items[] = {{"user", "Hesabım", "account"}, {"settings", "Ayarlar", "settings"}, {"key", "API ve Bağlantılar", "api"}, {"server", "Model Yönetimi", "models"}, {"cpu", "Donanım Bilgisi", "hardware"}, {"sun", "Görünüm", "appearance"}};
        auto *hd = mkLabel(UM()->user().email, "small"); hd->setContentsMargins(12, 6, 0, 4); pop->body()->addWidget(hd);
        for (const E &e : items) {
            auto *r = new MenuRow(QString::fromLatin1(e.icon), QString::fromUtf8(e.text)); r->setMinimumWidth(m_full - 60); const QString key = QString::fromLatin1(e.key);
            connect(r, &QAbstractButton::clicked, this, [this, pop, key] { pop->close(); emit accountAction(key); }); pop->body()->addWidget(r);
        }
        auto *sep = new QFrame; sep->setFixedHeight(1); sep->setStyleSheet(QStringLiteral("background:%1;").arg(g_t.border.name())); pop->body()->addWidget(sep);
        auto *out = new MenuRow("logout", QStringLiteral("Çıkış Yap")); out->setMinimumWidth(m_full - 60);
        connect(out, &QAbstractButton::clicked, this, [this, pop] { pop->close(); emit accountAction("signout"); }); pop->body()->addWidget(out);
        pop->showNear(m_profile, true);
    }
    QWidget *m_inner; QWidget *m_searchWrap; QLineEdit *m_search; FlatButton *m_searchBtn; SideList *m_list; ProfileButton *m_profile; QPropertyAnimation *m_anim; QTimer *m_deb;
    int m_w = 340, m_full = 340; bool m_open = true; QString m_activeConv, m_activeSection, m_project;
};

class ChatHeader : public QWidget {
    Q_OBJECT
public:
    explicit ChatHeader(QWidget *p = nullptr) : QWidget(p) {
        setFixedHeight(56);
        auto *h = hb(this, 0, 4); h->setContentsMargins(12, 0, 12, 0);
        m_toggle = fb("sidebar", QString(), this)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Kenar çubuğunu aç (Ctrl+B)"));
        m_new = fb("newchat", QString(), this)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Yeni sohbet (Ctrl+N)"));
        m_title = fb(QString(), QStringLiteral("Yeni sohbet"), this)->strong()->trail("chevdown"); m_title->setMaximumWidth(380);
        m_model = new ModelSelector(this);
        auto *share = fb("share", QString(), this)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Paylaş / dışa aktar"));
        auto *more = fb("more", QString(), this)->isz(20)->fixed(36, 36)->tip(QStringLiteral("Diğer işlemler"));
        h->addWidget(m_toggle); h->addWidget(m_new); h->addWidget(m_title); h->addWidget(m_model); h->addStretch(1); h->addWidget(share); h->addWidget(more);
        setSidebarOpen(true);
        connect(m_toggle, &QAbstractButton::clicked, this, &ChatHeader::toggleSidebar);
        connect(m_new, &QAbstractButton::clicked, this, &ChatHeader::newChat);
        connect(m_title, &QAbstractButton::clicked, this, [this] { convMenuAt(m_title); });
        connect(more, &QAbstractButton::clicked, this, [this, more] { convMenuAt(more, true); });
        connect(share, &QAbstractButton::clicked, this, [this, share] {
            QMenu m(this);
            QAction *a1 = m.addAction(QIcon(iconPix("copy", 18, g_t.text)), QStringLiteral("Sohbeti panoya kopyala (Markdown)")); QAction *a2 = m.addAction(QIcon(iconPix("share", 18, g_t.text)), QStringLiteral("Markdown dosyası olarak kaydet…"));
            a1->setEnabled(m_has); a2->setEnabled(m_has);
            QAction *r = m.exec(share->mapToGlobal(QPoint(0, share->height() + 4)));
            if (r == a1) emit shareCopy(); else if (r == a2) emit shareFile();
        });
    }
    void setTitle(const QString &t, bool hasConv) { m_has = hasConv; m_title->setText(t.isEmpty() ? QStringLiteral("Yeni sohbet") : t); m_title->trail(hasConv ? "chevdown" : QString()); m_title->updateGeometry(); }
    void setPinned(bool p) { m_pinned = p; }
    void setSidebarOpen(bool open) { m_toggle->setVisible(!open); m_new->setVisible(!open); }
signals:
    void toggleSidebar();
    void newChat();
    void renameRequested();
    void pinToggle();
    void deleteRequested();
    void shareCopy();
    void shareFile();
private:
    void convMenuAt(QWidget *anchor, bool withNew = false) {
        if (!m_has && !withNew) return;
        QMenu m(this); QAction *aNew = nullptr;
        if (withNew) { aNew = m.addAction(QIcon(iconPix("newchat", 18, g_t.text)), QStringLiteral("Yeni sohbet")); m.addSeparator(); }
        QAction *aRen = m.addAction(QIcon(iconPix("newchat", 18, g_t.text)), QStringLiteral("Yeniden adlandır"));
        QAction *aPin = m.addAction(QIcon(iconPix("pin", 18, g_t.text)), m_pinned ? QStringLiteral("Sabitlemeyi kaldır") : QStringLiteral("Sabitle"));
        m.addSeparator(); QAction *aDel = m.addAction(QIcon(iconPix("trash", 18, g_t.danger)), QStringLiteral("Sohbeti sil"));
        for (QAction *a : {aRen, aPin, aDel}) a->setEnabled(m_has);
        QAction *r = m.exec(anchor->mapToGlobal(QPoint(0, anchor->height() + 4)));
        if (!r) return;
        if (r == aNew) emit newChat(); else if (r == aRen) emit renameRequested(); else if (r == aPin) emit pinToggle(); else if (r == aDel) emit deleteRequested();
    }
    FlatButton *m_toggle, *m_new, *m_title; ModelSelector *m_model; bool m_has = false, m_pinned = false;
public:
    ModelSelector *modelSelector() const { return m_model; }
};

// ============================================================================
// 9) Sohbet alanı (mesaj akışı, üretim, dışa aktarma)
// ============================================================================
class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *p = nullptr) : QWidget(p) {
        auto *v = vb(this);
        m_header = new ChatHeader(this);
        m_stack = new QStackedWidget(this);
        // karşılama sayfası
        m_welcome = new QWidget; auto *wv = vb(m_welcome, 24, 12); wv->setAlignment(Qt::AlignHCenter);
        wv->addStretch(5);
        auto *logo = new LogoLabel(60, m_welcome); wv->addWidget(logo, 0, Qt::AlignHCenter);
        auto *title = mkLabel(QStringLiteral("Nasıl yardımcı olabilirim?"), "h1", m_welcome); title->setStyleSheet(QStringLiteral("font-size:30px;font-weight:600;")); title->setAlignment(Qt::AlignCenter); wv->addWidget(title);
        m_hint = mkLabel(QString(), "muted", m_welcome); m_hint->setAlignment(Qt::AlignCenter); m_hint->setWordWrap(true); m_hint->setMaximumWidth(560); wv->addWidget(m_hint, 0, Qt::AlignHCenter);
        m_cta = fb("settings", QStringLiteral("API ve Bağlantıları aç"), m_welcome)->kind(FlatButton::Outline)->isz(18)->fixed(230, 38); wv->addWidget(m_cta, 0, Qt::AlignHCenter);
        wv->addStretch(6);
        // mesaj sayfası
        m_scroll = new SmoothScroll(this); m_host = new QWidget;
        auto *hh = hb(m_host); hh->addStretch(1);
        m_col = new QWidget(m_host); m_col->setMaximumWidth(800); hh->addWidget(m_col, 100); hh->addStretch(1);
        m_colLay = vb(m_col); m_colLay->setContentsMargins(20, 12, 20, 28); m_colLay->setSpacing(26); m_colLay->addStretch(1);
        m_scroll->setWidget(m_host);
        m_stack->addWidget(m_welcome); m_stack->addWidget(m_scroll);
        // composer
        auto *cw = new QWidget(this); auto *ch = hb(cw); ch->setContentsMargins(12, 0, 12, 0); ch->addStretch(1);
        m_comp = new MessageComposer(cw); m_comp->setMaximumWidth(796); ch->addWidget(m_comp, 100); ch->addStretch(1);
        auto *foot = mkLabel(QStringLiteral("RÜZGAR AI hata yapabilir. Önemli bilgileri kontrol edin."), "small", this); foot->setAlignment(Qt::AlignCenter); foot->setContentsMargins(0, 0, 0, 8);
        v->addWidget(m_header); v->addWidget(m_stack, 1); v->addWidget(cw); v->addWidget(foot);
        m_down = fb("arrowdown", QString(), this)->kind(FlatButton::Raised)->isz(18)->rad(-1)->fixed(36, 36)->tip(QStringLiteral("En alta in")); m_down->hide();
        m_flush = new QTimer(this); m_flush->setSingleShot(true); m_flush->setInterval(40);
        connect(m_flush, &QTimer::timeout, this, [this] { if (m_cur) m_cur->setStreaming(m_buf); });
        m_stack->installEventFilter(this);
        connect(m_comp, &MessageComposer::sendRequested, this, &ChatWidget::onSend);
        connect(m_comp, &MessageComposer::stopRequested, this, &ChatWidget::stopGeneration);
        connect(m_comp, &MessageComposer::heightChanged, this, [this] { layoutOverlay(); });
        connect(m_scroll, &SmoothScroll::bottomState, this, [this](bool atBottom) { setDownVisible(!atBottom && !m_msgs.isEmpty()); });
        connect(m_down, &QAbstractButton::clicked, this, [this] { m_scroll->scrollToBottom(true); });
        connect(m_cta, &QAbstractButton::clicked, this, [this] { emit openSettingsRequested(4); });
        connect(MC(), &ModelCatalog::activeChanged, this, [this] { refreshModelState(); });
        connect(MC(), &ModelCatalog::changed, this, [this] { refreshModelState(); });
        connect(CM(), &ConversationManager::changed, this, [this] { updateHeader(); });
        connect(m_header, &ChatHeader::toggleSidebar, this, &ChatWidget::toggleSidebarRequested);
        connect(m_header, &ChatHeader::newChat, this, [this] { newConversation(); emit conversationActivated(QString()); });
        connect(m_header, &ChatHeader::renameRequested, this, [this] {
            if (m_convId.isEmpty()) return; QString t = CM()->get(m_convId).title;
            if (askText(window(), QStringLiteral("Sohbeti yeniden adlandır"), QStringLiteral("Yeni ad:"), &t) && !t.isEmpty()) CM()->rename(m_convId, t);
        });
        connect(m_header, &ChatHeader::pinToggle, this, [this] { if (!m_convId.isEmpty()) CM()->setPinned(m_convId, !CM()->get(m_convId).pinned); });
        connect(m_header, &ChatHeader::deleteRequested, this, [this] {
            if (m_convId.isEmpty()) return;
            if (confirmBox(window(), QStringLiteral("Sohbeti sil"), QStringLiteral("“%1” sohbeti silinsin mi? Bu işlem geri alınamaz.").arg(CM()->get(m_convId).title))) {
                const QString id = m_convId; newConversation(); CM()->remove(id); emit convDeleted(id); emit conversationActivated(QString());
            }
        });
        connect(m_header, &ChatHeader::shareCopy, this, [this] { QGuiApplication::clipboard()->setText(exportMd()); toast(this, QStringLiteral("Sohbet Markdown olarak panoya kopyalandı.")); });
        connect(m_header, &ChatHeader::shareFile, this, [this] { saveMd(); });
        newConversation(); refreshModelState();
    }
    ChatHeader *header() const { return m_header; }
    QString currentId() const { return m_convId; }
    bool isGenerating() const { return m_job != nullptr; }
    void focusInput() { m_comp->focusInput(); }
    void newConversation() {
        stopGeneration(); clearMessages(); m_convId.clear(); setDownVisible(false);
        m_stack->setCurrentWidget(m_welcome); fadeWidget(m_welcome, true, 420);
        updateHeader(); refreshModelState(); m_comp->focusInput();
    }
    void openConversation(const QString &id) {
        if (id.isEmpty() || CM()->get(id).id.isEmpty()) { newConversation(); return; }
        if (id == m_convId && !m_msgs.isEmpty()) return;
        stopGeneration(); clearMessages(); m_convId = id;
        const QList<MsgRec> ms = CM()->messages(id);
        for (const MsgRec &m : ms) addMessageWidget(m, false);
        updateLast(); updateHeader(); Cfg::set("lastConv", id);
        m_stack->setCurrentWidget(ms.isEmpty() ? m_welcome : static_cast<QWidget *>(m_scroll));
        m_scroll->setStick(true); QTimer::singleShot(0, this, [this] { m_scroll->scrollToBottom(false); });
        m_comp->focusInput();
    }
    void stopGeneration() { if (!m_job) return; ApiJob *j = m_job; j->abort(); if (m_job == j) onJobEnd(j, QString(), false); }
    void reloadConversation() { const QString id = m_convId; m_convId.clear(); if (id.isEmpty()) newConversation(); else openConversation(id); }
signals:
    void conversationActivated(const QString &id);
    void openSettingsRequested(int page);
    void toggleSidebarRequested();
    void convDeleted(const QString &id);
protected:
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e); layoutOverlay(); }
    bool eventFilter(QObject *o, QEvent *e) override { if (o == m_stack && (e->type() == QEvent::Resize || e->type() == QEvent::Move)) layoutOverlay(); return QWidget::eventFilter(o, e); }
private:
    void layoutOverlay() { const QRect g = m_stack->geometry(); m_down->move(g.center().x() - 18, g.bottom() - 46); m_down->raise(); }
    void setDownVisible(bool on) { if (on == m_downShown) return; m_downShown = on; layoutOverlay(); fadeWidget(m_down, on, 150); }
    void updateHeader() {
        if (m_convId.isEmpty()) { m_header->setTitle(QStringLiteral("Yeni sohbet"), false); m_header->setPinned(false); }
        else { const ConvInfo c = CM()->get(m_convId); m_header->setTitle(c.title, true); m_header->setPinned(c.pinned); }
    }
    void refreshModelState() {
        const ModelEntry e = MC()->active(); bool ok = false; const ProviderCfg c = MC()->resolveProvider(e, &ok);
        m_comp->setThinkingSupport(ok && thinkingSupported(c.kind, MC()->apiModelId(e, c)));
        int lv = 0; const QString st = MC()->statusOf(e, &lv);
        const bool needKey = ok && (c.kind == PK::OpenAI || c.kind == PK::Anthropic || c.kind == PK::DeepSeek || c.kind == PK::Gemini) && AM()->keyFor(c).isEmpty();
        if (!ok) m_hint->setText(QStringLiteral("Seçili model (%1) için bir sağlayıcı yapılandırılmamış. Başlamak için API ve Bağlantılar bölümünden bir sağlayıcı ekleyin.").arg(e.name));
        else if (needKey) m_hint->setText(QStringLiteral("%1 için API anahtarı girilmemiş. API ve Bağlantılar bölümünden ekleyin.").arg(c.name));
        else m_hint->setText(QStringLiteral("Etkin model: %1 · %2").arg(e.name, st));
        m_cta->setVisible(!ok || needKey);
    }
    void clearMessages() {
        for (MessageWidget *w : m_msgs) { m_colLay->removeWidget(w); w->hide(); w->deleteLater(); }
        m_msgs.clear(); m_cur = nullptr;
    }
    void removeWidget(MessageWidget *w) { m_msgs.removeAll(w); if (m_cur == w) m_cur = nullptr; m_colLay->removeWidget(w); w->hide(); w->deleteLater(); }
    void updateLast() { for (int i = 0; i < m_msgs.size(); ++i) m_msgs[i]->setLast(i == m_msgs.size() - 1 && !m_msgs[i]->isUser()); }
    MessageWidget *addMessageWidget(const MsgRec &m, bool animate) {
        auto *w = new MessageWidget(m, m_col);
        m_colLay->insertWidget(m_colLay->count() - 1, w); m_msgs << w;
        connect(w, &MessageWidget::regenerate, this, [this] { regenerateLast(); });
        connect(w, &MessageWidget::retry, this, [this] { regenerateLast(); });
        connect(w, &MessageWidget::openSettings, this, [this] { emit openSettingsRequested(4); });
        connect(w, &MessageWidget::feedbackChanged, this, [](qint64 id, int f) { if (id > 0) CM()->setFeedback(id, f); });
        connect(w, &MessageWidget::deleteRequested, this, [this, w] {
            if (m_job && w == m_cur) return;
            if (w->dbId() > 0) CM()->deleteMessage(w->dbId());
            removeWidget(w); updateLast(); if (m_msgs.isEmpty()) { m_stack->setCurrentWidget(m_welcome); fadeWidget(m_welcome, true, 300); }
        });
        m_stack->setCurrentWidget(m_scroll);
        if (animate) w->playAppear();
        return w;
    }
    QString systemPrompt() const {
        QStringList parts; const QString lang = Cfg::get("lang", "auto").toString();
        if (lang == "tr") parts << QStringLiteral("Her zaman Türkçe yanıt ver."); else if (lang == "en") parts << QStringLiteral("Always respond in English.");
        const QString ins = UM()->user().instructions.trimmed();
        if (!ins.isEmpty()) parts << QStringLiteral("Kullanıcının özel talimatları:\n") + ins;
        return parts.join("\n\n");
    }
    void onSend(const QString &text, const QStringList &images) {
        if (m_job) return;
        const ModelEntry e = MC()->active(); bool ok = false; const ProviderCfg c = MC()->resolveProvider(e, &ok);
        if (!images.isEmpty() && ok && MC()->visionOf(e, c) == 0) {
            toast(this, QStringLiteral("“%1” görsel girdisini desteklemiyor. Görselleri kaldırın veya görsel destekli bir model seçin (sağlayıcı ayarlarında “Görsel desteği” seçeneği de değiştirilebilir). İstek gönderilmedi.").arg(e.name), true);
            return;
        }
        if (m_convId.isEmpty()) {
            m_convId = CM()->create(QStringLiteral("Yeni sohbet"), e.id);
            if (m_convId.isEmpty()) return;
            emit conversationActivated(m_convId);
        }
        MsgRec um; um.conv = m_convId; um.role = "user"; um.content = text; um.images = images; um.ts = nowMs();
        um.id = CM()->addMessage(um);
        if (um.id == 0) return;
        m_comp->clearInput();
        addMessageWidget(um, true); m_scroll->scrollToBottom(true);
        if (CM()->get(m_convId).title == QLatin1String("Yeni sohbet")) {
            QString t = text.simplified(); if (t.isEmpty()) t = QStringLiteral("Görsel sohbeti"); if (t.size() > 48) t = t.left(48).trimmed() + QStringLiteral("…");
            CM()->rename(m_convId, t);
        }
        generate();
    }
    void regenerateLast() {
        if (m_job || m_msgs.isEmpty()) return;
        MessageWidget *last = m_msgs.last();
        if (!last->isUser()) { if (last->dbId() > 0) CM()->deleteMessage(last->dbId()); removeWidget(last); }
        generate();
    }
    void generate() {
        const ModelEntry e = MC()->active(); bool ok = false; const ProviderCfg c = MC()->resolveProvider(e, &ok);
        MsgRec ar; ar.conv = m_convId; ar.role = "assistant"; ar.model = e.name;
        MessageWidget *mw = addMessageWidget(ar, true); m_cur = mw; mw->showTyping(true); updateLast(); m_scroll->scrollToBottom(true);
        auto fail = [&](const QString &msg) { mw->setError(msg, true, true); m_cur = nullptr; };
        if (!ok) { fail(QStringLiteral("Seçili model için bir sağlayıcı yapılandırılmamış. Ayarlar → API ve Bağlantılar bölümünden sağlayıcıyı ekleyin ve kaydedin.")); return; }
        const QString key = AM()->keyFor(c);
        if ((c.kind == PK::OpenAI || c.kind == PK::Anthropic || c.kind == PK::DeepSeek || c.kind == PK::Gemini) && key.trimmed().isEmpty()) { fail(QStringLiteral("%1 için API anahtarı girilmemiş (geçersiz anahtar).").arg(c.name)); return; }
        const QString verr = validateBaseUrl(c.baseUrl);
        if (!verr.isEmpty()) { fail(c.kind == PK::Local ? QStringLiteral("Geçersiz sunucu adresi: ") + c.baseUrl : verr); return; }
        const QString apiModel = MC()->apiModelId(e, c);
        if (apiModel.trimmed().isEmpty()) { fail(c.kind == PK::Local ? QStringLiteral("RÜZGAR Local Server için model seçilmedi. API ve Bağlantılar → RÜZGAR Local Server bölümünde bağlantıyı test edip bir model seçin.") : QStringLiteral("Model kimliği boş (eksik model).")); return; }
        const int vis = MC()->visionOf(e, c);
        QList<MsgRec> hist = CM()->messages(m_convId); const int maxH = Cfg::get("maxHist", 0).toInt();
        if (maxH > 0 && hist.size() > maxH) hist = hist.mid(hist.size() - maxH);
        QList<ChatMsg> msgs;
        for (const MsgRec &m : hist) {
            ChatMsg cm; cm.role = m.role; cm.text = m.content;
            if (!m.images.isEmpty()) {
                if (vis == 0) cm.text += QStringLiteral("\n[Kullanıcı bir görsel ekledi; seçili model görsel desteklemediği için gönderilmedi.]");
                else for (const QString &p : m.images) { if (QFileInfo::exists(p)) cm.images << p; else cm.text += QStringLiteral("\n[Bir görsel dosyası artık mevcut değil, gönderilemedi.]"); }
            }
            msgs << cm;
        }
        GenOpts o; o.thinking = m_comp->thinking(); o.maxTokens = Cfg::get("maxTokens", 4096).toInt(); o.system = systemPrompt();
        m_buf.clear(); m_gotAny = false; m_streamConv = m_convId; m_streamModel = e.name;
        ApiJob *job = AM()->chat(c, key, apiModel, msgs, o); m_job = job; m_comp->setGenerating(true);
        connect(job, &ApiJob::delta, this, [this, job](const QString &t) {
            if (m_job != job) return;
            m_buf += t;
            if (Cfg::get("streamUi", true).toBool()) { m_gotAny = true; if (m_cur) m_cur->showTyping(false); if (!m_flush->isActive()) m_flush->start(); }
        });
        connect(job, &ApiJob::finished, this, [this, job] { onJobEnd(job, QString(), false); });
        connect(job, &ApiJob::failed, this, [this, job](const QString &msg) { onJobEnd(job, msg, true); });
        job->start();
    }
    void onJobEnd(ApiJob *job, const QString &err, bool failed) {
        if (m_job != job) return;
        m_flush->stop(); m_job = nullptr; job->deleteLater(); m_comp->setGenerating(false);
        MessageWidget *mw = m_cur; m_cur = nullptr;
        if (!mw) return;
        if (!m_buf.trimmed().isEmpty()) {
            MsgRec ar; ar.conv = m_streamConv; ar.role = "assistant"; ar.content = m_buf; ar.model = m_streamModel;
            mw->setDbId(CM()->addMessage(ar)); mw->finish(m_buf);
            if (failed) mw->setError(err, true, true);
        } else if (failed) { mw->finish(QString()); mw->setError(err, true, true); }
        else { removeWidget(mw); }
        updateLast();
        if (m_msgs.isEmpty()) { m_stack->setCurrentWidget(m_welcome); }
    }
    QString exportMd() const {
        if (m_convId.isEmpty()) return QString();
        QString s = QStringLiteral("# ") + CM()->get(m_convId).title + QStringLiteral("\n\n");
        for (const MsgRec &m : CM()->messages(m_convId)) {
            s += (m.role == QLatin1String("user") ? QStringLiteral("**Siz:**\n\n") : QStringLiteral("**Asistan") + (m.model.isEmpty() ? QString() : QStringLiteral(" (") + m.model + QStringLiteral(")")) + QStringLiteral(":**\n\n")) + m.content + QStringLiteral("\n\n---\n\n");
        }
        return s;
    }
    void saveMd() {
        if (m_convId.isEmpty()) return;
        QString name = CM()->get(m_convId).title; name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), "_");
        const QString path = QFileDialog::getSaveFileName(window(), QStringLiteral("Sohbeti kaydet"), QDir::homePath() + "/" + name + ".md", QStringLiteral("Markdown (*.md)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { toast(this, QStringLiteral("Dosya yazılamadı: ") + f.errorString(), true); return; }
        f.write(exportMd().toUtf8()); f.close(); toast(this, QStringLiteral("Sohbet kaydedildi."));
    }
    ChatHeader *m_header; QStackedWidget *m_stack; QWidget *m_welcome, *m_host, *m_col; SmoothScroll *m_scroll; QVBoxLayout *m_colLay;
    MessageComposer *m_comp; FlatButton *m_down, *m_cta; QLabel *m_hint; QTimer *m_flush;
    QList<MessageWidget *> m_msgs; MessageWidget *m_cur = nullptr; ApiJob *m_job = nullptr; QString m_convId, m_buf, m_streamConv, m_streamModel; bool m_gotAny = false, m_downShown = false;
};

// ============================================================================
// 10) Hesap ekranları (giriş / kayıt)
// ============================================================================
static QLineEdit *pwField(QWidget *p, const QString &ph) {
    auto *le = new QLineEdit(p); le->setEchoMode(QLineEdit::Password); le->setPlaceholderText(ph);
    QAction *a = le->addAction(QIcon(iconPix("eye", 18, g_t.text2)), QLineEdit::TrailingPosition);
    a->setCheckable(true); a->setToolTip(QStringLiteral("Göster / gizle"));
    QObject::connect(a, &QAction::toggled, le, [le, a](bool on) { le->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password); a->setIcon(QIcon(iconPix(on ? "eyeoff" : "eye", 18, g_t.text2))); });
    return le;
}
static QLineEdit *textField(QWidget *p, const QString &ph, const QString &text = QString()) { auto *le = new QLineEdit(text, p); le->setPlaceholderText(ph); return le; }
static QPushButton *primaryBtn(const QString &t, QWidget *p = nullptr) { auto *b = new QPushButton(t, p); b->setProperty("kind", "primary"); b->setMinimumHeight(42); b->setCursor(Qt::PointingHandCursor); return b; }

class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *p = nullptr) : QWidget(p) {
        auto *v = vb(this, 0, 12);
        m_id = textField(this, QStringLiteral("E-posta veya kullanıcı adı")); m_id->setMinimumHeight(42);
        m_pw = pwField(this, QStringLiteral("Parola")); m_pw->setMinimumHeight(42);
        m_keep = new QCheckBox(QStringLiteral("Bu bilgisayarda oturumu açık tut"), this); m_keep->setChecked(Cfg::get("keepSignedIn", true).toBool());
        m_err = mkLabel(QString(), "err", this); m_err->setWordWrap(true); m_err->hide();
        m_btn = primaryBtn(QStringLiteral("Giriş yap"), this);
        auto *sw = fb(QString(), QStringLiteral("Hesabınız yok mu? Kayıt olun"), this)->strong()->fixed(240, 36); sw->tint(g_t.accent);
        v->addWidget(m_id); v->addWidget(m_pw); v->addWidget(m_keep); v->addWidget(m_err); v->addWidget(m_btn); v->addWidget(sw, 0, Qt::AlignHCenter);
        connect(m_btn, &QPushButton::clicked, this, [this] { submit(); });
        connect(m_pw, &QLineEdit::returnPressed, this, [this] { submit(); });
        connect(m_id, &QLineEdit::returnPressed, this, [this] { m_pw->setFocus(); });
        connect(sw, &QAbstractButton::clicked, this, &LoginWidget::switchToRegister);
    }
    void reset() { m_pw->clear(); m_err->hide(); m_id->setFocus(); }
signals:
    void loggedIn();
    void switchToRegister();
private:
    void submit() {
        m_err->hide(); m_btn->setEnabled(false); m_btn->setText(QStringLiteral("Giriş yapılıyor…"));
        QTimer::singleShot(30, this, [this] {
            Cfg::set("keepSignedIn", m_keep->isChecked());
            const QString e = UM()->login(m_id->text(), m_pw->text());
            m_btn->setEnabled(true); m_btn->setText(QStringLiteral("Giriş yap"));
            if (!e.isEmpty()) { m_err->setText(e); m_err->show(); m_pw->selectAll(); m_pw->setFocus(); return; }
            m_pw->clear(); emit loggedIn();
        });
    }
    QLineEdit *m_id, *m_pw; QCheckBox *m_keep; QLabel *m_err; QPushButton *m_btn;
};

class RegisterWidget : public QWidget {
    Q_OBJECT
public:
    explicit RegisterWidget(QWidget *p = nullptr) : QWidget(p) {
        auto *v = vb(this, 0, 10);
        auto *nr = new QWidget(this); auto *nh = hb(nr, 0, 10);
        m_first = textField(nr, QStringLiteral("Ad")); m_last = textField(nr, QStringLiteral("Soyad")); m_first->setMinimumHeight(42); m_last->setMinimumHeight(42); nh->addWidget(m_first); nh->addWidget(m_last);
        m_user = textField(this, QStringLiteral("Kullanıcı adı")); m_email = textField(this, QStringLiteral("E-posta")); m_pw = pwField(this, QStringLiteral("Parola (en az 8 karakter)")); m_pw2 = pwField(this, QStringLiteral("Parolayı doğrula"));
        for (QLineEdit *l : {m_user, m_email, m_pw, m_pw2}) l->setMinimumHeight(42);
        m_err = mkLabel(QString(), "err", this); m_err->setWordWrap(true); m_err->hide();
        m_btn = primaryBtn(QStringLiteral("Hesap oluştur"), this);
        auto *sw = fb(QString(), QStringLiteral("Zaten hesabınız var mı? Giriş yapın"), this)->strong()->fixed(270, 36); sw->tint(g_t.accent);
        v->addWidget(nr); v->addWidget(m_user); v->addWidget(m_email); v->addWidget(m_pw); v->addWidget(m_pw2); v->addWidget(m_err); v->addWidget(m_btn); v->addWidget(sw, 0, Qt::AlignHCenter);
        connect(m_btn, &QPushButton::clicked, this, [this] { submit(); });
        connect(m_pw2, &QLineEdit::returnPressed, this, [this] { submit(); });
        connect(sw, &QAbstractButton::clicked, this, &RegisterWidget::switchToLogin);
    }
    void reset() { for (QLineEdit *l : {m_first, m_last, m_user, m_email, m_pw, m_pw2}) l->clear(); m_err->hide(); m_first->setFocus(); }
signals:
    void registered();
    void switchToLogin();
private:
    void submit() {
        m_err->hide(); m_btn->setEnabled(false); m_btn->setText(QStringLiteral("Hesap oluşturuluyor…"));
        QTimer::singleShot(30, this, [this] {
            const QString e = UM()->registerUser(m_first->text(), m_last->text(), m_user->text(), m_email->text(), m_pw->text(), m_pw2->text());
            m_btn->setEnabled(true); m_btn->setText(QStringLiteral("Hesap oluştur"));
            if (!e.isEmpty()) { m_err->setText(e); m_err->show(); return; }
            for (QLineEdit *l : {m_pw, m_pw2}) l->clear(); emit registered();
        });
    }
    QLineEdit *m_first, *m_last, *m_user, *m_email, *m_pw, *m_pw2; QLabel *m_err; QPushButton *m_btn;
};

class AuthScreen : public QWidget {
    Q_OBJECT
public:
    explicit AuthScreen(QWidget *p = nullptr) : QWidget(p) {
        auto *root = vb(this);
        auto *sa = new QScrollArea(this); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
        auto *host = new QWidget; auto *hv = vb(host); hv->setContentsMargins(20, 30, 20, 30);
        auto *card = new QFrame(host); card->setObjectName("authCard"); card->setFixedWidth(460);
        auto *cv = vb(card, 36, 14);
        auto *logo = new LogoLabel(52, card); cv->addWidget(logo, 0, Qt::AlignHCenter);
        m_title = mkLabel(QString(), "h1", card); m_title->setAlignment(Qt::AlignCenter); m_sub = mkLabel(QString(), "muted", card); m_sub->setAlignment(Qt::AlignCenter); m_sub->setWordWrap(true);
        cv->addWidget(m_title); cv->addWidget(m_sub); cv->addSpacing(6);
        m_stack = new QStackedWidget(card); m_login = new LoginWidget(m_stack); m_reg = new RegisterWidget(m_stack); m_stack->addWidget(m_login); m_stack->addWidget(m_reg); cv->addWidget(m_stack);
        auto *note = mkLabel(QStringLiteral("Bu hesap yalnızca bu bilgisayarda saklanır ve cihazlar arasında eşitlenmez. Parolalar tuzlanmış PBKDF2-SHA256 ile özetlenir; düz metin olarak saklanmaz."), "small", card); note->setWordWrap(true); note->setAlignment(Qt::AlignCenter);
        cv->addSpacing(4); cv->addWidget(note);
        hv->addStretch(1); hv->addWidget(card, 0, Qt::AlignHCenter); hv->addStretch(1);
        sa->setWidget(host); root->addWidget(sa);
        connect(m_login, &LoginWidget::loggedIn, this, &AuthScreen::authenticated);
        connect(m_reg, &RegisterWidget::registered, this, &AuthScreen::authenticated);
        connect(m_login, &LoginWidget::switchToRegister, this, [this] { showMode(true); });
        connect(m_reg, &RegisterWidget::switchToLogin, this, [this] { showMode(false); });
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
    void start() { showMode(!UM()->hasAccounts(), false); }
signals:
    void authenticated();
protected:
    void paintEvent(QPaintEvent *) override { QPainter p(this); p.fillRect(rect(), g_t.sidebar); }
private:
    void showMode(bool reg, bool animate = true) {
        if (reg) { m_title->setText(QStringLiteral("Hesap oluşturun")); m_sub->setText(QStringLiteral("RÜZGAR AI'yı kullanmaya başlamak için yerel bir hesap oluşturun.")); m_reg->reset(); m_stack->setCurrentWidget(m_reg); if (animate) fadeWidget(m_reg, true, 220); }
        else { m_title->setText(QStringLiteral("Tekrar hoş geldiniz")); m_sub->setText(QStringLiteral("RÜZGAR AI yerel hesabınıza giriş yapın.")); m_login->reset(); m_stack->setCurrentWidget(m_login); if (animate) fadeWidget(m_login, true, 220); }
    }
    QLabel *m_title, *m_sub; QStackedWidget *m_stack; LoginWidget *m_login; RegisterWidget *m_reg;
};

// ============================================================================
// 11) Ayarlar penceresi (9 kategori)
// ============================================================================
static QFrame *hline() { auto *f = new QFrame; f->setObjectName("hline"); f->setFixedHeight(1); return f; }
static QWidget *settingRow(const QString &title, const QString &desc, QWidget *ctl) {
    auto *w = new QWidget; auto *h = hb(w, 0, 18); h->setContentsMargins(0, 12, 0, 12);
    auto *l = new QWidget(w); auto *lv = vb(l, 0, 2); lv->addWidget(new QLabel(title, l));
    if (!desc.isEmpty()) { auto *d = mkLabel(desc, "small", l); d->setWordWrap(true); lv->addWidget(d); }
    h->addWidget(l, 1); h->addWidget(ctl, 0, Qt::AlignRight | Qt::AlignVCenter);
    return w;
}
static QSpinBox *numSpin(int lo, int hi, int v, int w = 110) { auto *s = new QSpinBox; s->setRange(lo, hi); s->setValue(v); s->setButtonSymbols(QAbstractSpinBox::NoButtons); s->setFixedWidth(w); return s; }
static void setStatus(QLabel *l, const QString &t, int lv) {
    l->setText(t); l->setWordWrap(true); l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->setStyleSheet(QStringLiteral("color:%1;").arg((lv == 1 ? g_t.ok : (lv == 2 ? g_t.danger : g_t.text2)).name()));
}

class ThemeCard : public QAbstractButton {
    Q_OBJECT
public:
    ThemeCard(int mode, const QString &label, QWidget *p = nullptr) : QAbstractButton(p), m_mode(mode), m_label(label) {
        setCheckable(true); setFixedSize(168, 128); setCursor(Qt::PointingHandCursor); setFocusPolicy(Qt::NoFocus);
        connect(&ThemeBus::I(), &ThemeBus::changed, this, [this] { update(); });
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QRectF pr(3, 3, width() - 6, height() - 40);
        auto prev = [&](const Theme &t, const QRectF &r) {
            QPainterPath path; path.addRoundedRect(r, 10, 10); p.save(); p.setClipPath(path);
            p.fillRect(r, t.bg); p.fillRect(QRectF(r.left(), r.top(), r.width() * 0.27, r.height()), t.sidebar);
            p.setPen(Qt::NoPen); p.setBrush(t.hover);
            p.drawRoundedRect(QRectF(r.left() + r.width() * 0.33, r.top() + 14, r.width() * 0.4, 8), 4, 4);
            p.drawRoundedRect(QRectF(r.left() + r.width() * 0.33, r.top() + 30, r.width() * 0.55, 8), 4, 4);
            p.setBrush(t.composer); p.setPen(QPen(t.border, 1)); p.drawRoundedRect(QRectF(r.left() + r.width() * 0.33, r.bottom() - 26, r.width() * 0.6, 16), 8, 8);
            p.setPen(Qt::NoPen); p.setBrush(t.accent); p.drawEllipse(QPointF(r.right() - 16, r.bottom() - 18), 5, 5);
            p.restore();
        };
        const Theme lt = makeTheme(false), dk = makeTheme(true);
        if (m_mode == 0) prev(lt, pr); else if (m_mode == 1) prev(dk, pr);
        else { p.save(); p.setClipRect(QRectF(pr.left(), pr.top(), pr.width() / 2, pr.height())); prev(lt, pr); p.restore(); p.save(); p.setClipRect(QRectF(pr.center().x(), pr.top(), pr.width() / 2, pr.height())); prev(dk, pr); p.restore(); }
        p.setBrush(Qt::NoBrush); p.setPen(QPen(isChecked() ? g_t.accent : g_t.border, isChecked() ? 2.5 : 1)); p.drawRoundedRect(pr, 10, 10);
        QFont f = font(); f.setWeight(isChecked() ? QFont::DemiBold : QFont::Normal); p.setFont(f); p.setPen(g_t.text);
        p.drawText(QRectF(0, height() - 30, width(), 26), Qt::AlignCenter, m_label);
    }
    int m_mode; QString m_label;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent, int page) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Ayarlar")); resize(1040, 720); setMinimumSize(840, 580); setModal(true);
        auto *root = hb(this);
        auto *nav = new QWidget(this); nav->setFixedWidth(256); auto *nv = vb(nav, 16, 4);
        auto *tl = mkLabel(QStringLiteral("Ayarlar"), "h2", nav); tl->setContentsMargins(10, 6, 0, 12); nv->addWidget(tl);
        m_stack = new QStackedWidget(this);
        struct Cat { const char *icon, *name; };
        static const Cat cats[9] = {{"settings", "Genel"}, {"sun", "Görünüm"}, {"user", "Kişiselleştirme"}, {"server", "Modeller"}, {"key", "API ve Bağlantılar"}, {"cpu", "Donanım"}, {"chat", "Sohbet"}, {"user", "Hesap"}, {"info", "RÜZGAR AI Hakkında"}};
        m_grp = new QButtonGroup(this); m_grp->setExclusive(true);
        for (int i = 0; i < 9; ++i) {
            FlatButton *b = fb(QString::fromLatin1(cats[i].icon), QString::fromUtf8(cats[i].name), nav)->leftAlign()->strong()->isz(20); b->setCheckable(true); b->setFixedHeight(40);
            nv->addWidget(b); m_grp->addButton(b, i);
        }
        nv->addStretch(1);
        auto *div = new QFrame(this); div->setObjectName("hline"); div->setFixedWidth(1);
        root->addWidget(nav); root->addWidget(div); root->addWidget(m_stack, 1);
        buildGeneral(); buildAppearance(); buildPersonal(); buildModels(); buildApi(); buildHardware(); buildChat(); buildAccount(); buildAbout();
        connect(m_grp, &QButtonGroup::idClicked, this, [this](int i) { go(i, true); });
        go(qBound(0, page, 8), false);
    }
    bool signOutRequested() const { return m_signOut; }
    bool conversationsCleared() const { return m_cleared; }
protected:
    void showEvent(QShowEvent *e) override {
        QDialog::showEvent(e);
        if (Cfg::anim() && !m_shown) { m_shown = true; setWindowOpacity(0.0); auto *a = new QPropertyAnimation(this, "windowOpacity", this); a->setDuration(180); a->setStartValue(0.0); a->setEndValue(1.0); a->start(QAbstractAnimation::DeleteWhenStopped); }
    }
private:
    void go(int i, bool animate) { m_stack->setCurrentIndex(i); if (auto *b = m_grp->button(i)) b->setChecked(true); if (animate) fadeWidget(m_stack->currentWidget(), true, 190); }
    QVBoxLayout *mkPage(const QString &title, const QString &sub) {
        auto *sa = new QScrollArea; sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame); sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *w = new QWidget; auto *l = vb(w, 0, 6); l->setContentsMargins(36, 30, 36, 30);
        l->addWidget(mkLabel(title, "h1"));
        if (!sub.isEmpty()) { auto *s = mkLabel(sub, "muted"); s->setWordWrap(true); l->addWidget(s); }
        l->addSpacing(10); sa->setWidget(w); m_stack->addWidget(sa); return l;
    }
    QPushButton *pbtn(const QString &t, const char *kind = nullptr) { auto *b = new QPushButton(t); if (kind) b->setProperty("kind", QString::fromLatin1(kind)); b->setCursor(Qt::PointingHandCursor); return b; }

    // ---- 1) Genel ----
    void buildGeneral() {
        auto *l = mkPage(QStringLiteral("Genel"), QStringLiteral("Uygulamanın genel davranışları. Değişiklikler hemen kaydedilir."));
        auto *lang = new QComboBox; lang->addItems({QStringLiteral("Otomatik (modelin kararı)"), QStringLiteral("Türkçe"), QStringLiteral("English")}); lang->setFixedWidth(240);
        const QString cl = Cfg::get("lang", "auto").toString(); lang->setCurrentIndex(cl == "tr" ? 1 : (cl == "en" ? 2 : 0));
        connect(lang, &QComboBox::currentIndexChanged, this, [](int i) { Cfg::set("lang", i == 1 ? "tr" : (i == 2 ? "en" : "auto")); });
        l->addWidget(settingRow(QStringLiteral("Yanıt dili"), QStringLiteral("Seçim, sistem istemiyle modele iletilir. Arayüz dili şu an Türkçe'dir."), lang)); l->addWidget(hline());
        auto *st = new QComboBox; st->addItems({QStringLiteral("Yeni sohbetle başla"), QStringLiteral("Son sohbeti aç")}); st->setFixedWidth(240); st->setCurrentIndex(Cfg::get("startup", 0).toInt());
        connect(st, &QComboBox::currentIndexChanged, this, [](int i) { Cfg::set("startup", i); });
        l->addWidget(settingRow(QStringLiteral("Başlangıç davranışı"), QStringLiteral("Uygulama açıldığında ne gösterileceği."), st)); l->addWidget(hline());
        auto *keep = new QCheckBox; keep->setChecked(Cfg::get("keepSignedIn", true).toBool());
        connect(keep, &QCheckBox::toggled, this, [](bool b) { Cfg::set("keepSignedIn", b); });
        l->addWidget(settingRow(QStringLiteral("Oturumu açık tut"), QStringLiteral("Bu bilgisayarda son kullanıcı otomatik açılır. Parola saklanmaz; yalnızca hesap kimliği hatırlanır."), keep)); l->addWidget(hline());
        auto *an = new QCheckBox; an->setChecked(Cfg::anim());
        connect(an, &QCheckBox::toggled, this, [](bool b) { Cfg::set("anim", b); });
        l->addWidget(settingRow(QStringLiteral("Animasyonlar"), QStringLiteral("Kapatıldığında geçişler ve açılır pencere animasyonları devre dışı kalır."), an));
        l->addStretch(1);
    }
    // ---- 2) Görünüm ----
    void buildAppearance() {
        auto *l = mkPage(QStringLiteral("Görünüm"), QStringLiteral("Tema seçimi kaydedilir ve yeniden başlatmada hatırlanır."));
        auto *row = new QWidget; auto *h = hb(row, 0, 16); auto *g = new QButtonGroup(this); const int cur = Cfg::get("theme", 0).toInt();
        const QStringList names = {QStringLiteral("Açık"), QStringLiteral("Koyu"), QStringLiteral("Sistem")};
        for (int i = 0; i < 3; ++i) {
            auto *c = new ThemeCard(i, names[i], row); c->setChecked(i == cur); g->addButton(c, i); h->addWidget(c);
        }
        h->addStretch(1); g->setExclusive(true);
        connect(g, &QButtonGroup::idClicked, this, [this](int i) { Cfg::set("theme", i); switchTheme(this); });
        l->addSpacing(8); l->addWidget(row); l->addStretch(1);
    }
    // ---- 3) Kişiselleştirme ----
    void buildPersonal() {
        auto *l = mkPage(QStringLiteral("Kişiselleştirme"), QStringLiteral("Görünen adınız, avatar baş harfleriniz ve modele iletilecek özel talimatlar."));
        const UserRec u = UM()->user();
        m_avatar = new AvatarWidget(64); m_avatar->setInitials(u.initials);
        auto *top = new QWidget; auto *th = hb(top, 0, 16); th->addWidget(m_avatar); auto *tv = new QVBoxLayout; tv->addWidget(new QLabel(u.fullName())); tv->addWidget(mkLabel(u.email, "small")); th->addLayout(tv); th->addStretch(1);
        l->addWidget(top); l->addSpacing(6);
        m_pFirst = textField(nullptr, QStringLiteral("Ad"), u.first); m_pLast = textField(nullptr, QStringLiteral("Soyad"), u.last); m_pIni = textField(nullptr, QStringLiteral("Otomatik"), UserManager::initialsOf(u.first, u.last) == u.initials ? QString() : u.initials);
        m_pIni->setMaxLength(2); m_pIni->setFixedWidth(90); m_pFirst->setFixedWidth(240); m_pLast->setFixedWidth(240);
        connect(m_pIni, &QLineEdit::textChanged, this, [this] { m_avatar->setInitials(m_pIni->text().isEmpty() ? UserManager::initialsOf(m_pFirst->text(), m_pLast->text()) : QLocale(QLocale::Turkish).toUpper(m_pIni->text())); });
        connect(m_pFirst, &QLineEdit::textChanged, this, [this] { if (m_pIni->text().isEmpty()) m_avatar->setInitials(UserManager::initialsOf(m_pFirst->text(), m_pLast->text())); });
        connect(m_pLast, &QLineEdit::textChanged, this, [this] { if (m_pIni->text().isEmpty()) m_avatar->setInitials(UserManager::initialsOf(m_pFirst->text(), m_pLast->text())); });
        l->addWidget(settingRow(QStringLiteral("Ad"), QString(), m_pFirst)); l->addWidget(hline());
        l->addWidget(settingRow(QStringLiteral("Soyad"), QString(), m_pLast)); l->addWidget(hline());
        l->addWidget(settingRow(QStringLiteral("Avatar baş harfleri"), QStringLiteral("Boş bırakırsanız addan ve soyaddan otomatik üretilir (en fazla 2 harf)."), m_pIni)); l->addWidget(hline());
        l->addWidget(mkLabel(QStringLiteral("Özel talimatlar"), "h2"));
        auto *d = mkLabel(QStringLiteral("Her sohbette sistem istemi olarak modele gönderilir. Hassas bilgi yazmayın."), "small"); d->setWordWrap(true); l->addWidget(d);
        m_pIns = new QTextEdit; m_pIns->setObjectName("field"); m_pIns->setAcceptRichText(false); m_pIns->setPlainText(u.instructions); m_pIns->setMinimumHeight(120); m_pIns->setMaximumHeight(180);
        l->addWidget(m_pIns);
        m_pStatus = new QLabel; auto *save = pbtn(QStringLiteral("Kaydet"), "primary"); save->setFixedWidth(120);
        connect(save, &QPushButton::clicked, this, [this] {
            const UserRec cu = UM()->user();
            QString e = UM()->updateProfile(m_pFirst->text(), m_pLast->text(), cu.username, cu.email, m_pIni->text());
            if (e.isEmpty()) e = UM()->setInstructions(m_pIns->toPlainText());
            setStatus(m_pStatus, e.isEmpty() ? QStringLiteral("Kaydedildi.") : e, e.isEmpty() ? 1 : 2);
        });
        l->addWidget(save); l->addWidget(m_pStatus); l->addStretch(1);
    }
    // ---- 4) Modeller ----
    void buildModels() {
        auto *l = mkPage(QStringLiteral("Modeller"), QStringLiteral("Model listesi, etkin (varsayılan) model, bellek tahmini ve uyumluluk. Bellek değerleri tahmindir; bilinmeyenler “Bilinmiyor” olarak gösterilir."));
        auto *row = new QWidget; auto *rh = hb(row, 0, 20);
        m_modelList = new QListWidget; m_modelList->setFixedWidth(290); m_modelList->setMinimumHeight(430);
        auto *right = new QWidget; auto *rv = vb(right, 0, 12);
        m_modelDetail = new QLabel; m_modelDetail->setWordWrap(true); m_modelDetail->setTextFormat(Qt::RichText); m_modelDetail->setAlignment(Qt::AlignTop | Qt::AlignLeft); m_modelDetail->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        rv->addWidget(m_modelDetail); rv->addWidget(mkLabel(QStringLiteral("Anlık bellek"), "small")); rv->addWidget(new HardwarePanel(true));
        auto *br = new QWidget; auto *bh = hb(br, 0, 8);
        m_mAct = pbtn(QStringLiteral("Etkin model yap"), "primary"); auto *add = pbtn(QStringLiteral("Özel model ekle…")); m_mEdit = pbtn(QStringLiteral("Düzenle…")); m_mDel = pbtn(QStringLiteral("Sil"), "danger");
        bh->addWidget(m_mAct); bh->addWidget(add); bh->addWidget(m_mEdit); bh->addWidget(m_mDel); bh->addStretch(1); rv->addWidget(br); rv->addStretch(1);
        rh->addWidget(m_modelList); rh->addWidget(right, 1); l->addWidget(row); l->addStretch(1);
        connect(m_modelList, &QListWidget::currentRowChanged, this, [this] { showModel(); });
        connect(m_mAct, &QPushButton::clicked, this, [this] { if (auto *it = m_modelList->currentItem()) MC()->setActive(it->data(Qt::UserRole).toString()); });
        connect(add, &QPushButton::clicked, this, [this] { ModelEditDialog d(ModelEntry(), true, this); if (d.exec() == QDialog::Accepted) { const QString id = MC()->addCustom(d.result()); refreshModels(id); } });
        connect(m_mEdit, &QPushButton::clicked, this, [this] {
            auto *it = m_modelList->currentItem(); if (!it) return; ModelEntry e = MC()->byId(it->data(Qt::UserRole).toString()); if (e.builtin || e.id.startsWith("cfg:")) return;
            ModelEditDialog d(e, false, this); if (d.exec() == QDialog::Accepted) MC()->updateCustom(d.result());
        });
        connect(m_mDel, &QPushButton::clicked, this, [this] {
            auto *it = m_modelList->currentItem(); if (!it) return; const ModelEntry e = MC()->byId(it->data(Qt::UserRole).toString()); if (e.builtin || e.id.startsWith("cfg:")) return;
            if (confirmBox(this, QStringLiteral("Modeli sil"), QStringLiteral("“%1” özel modeli silinsin mi?").arg(e.name))) MC()->removeCustom(e.id);
        });
        connect(MC(), &ModelCatalog::changed, this, [this] { refreshModels(); });
        refreshModels();
    }
    void refreshModels(const QString &select = QString()) {
        QString keep = select; if (keep.isEmpty() && m_modelList->currentItem()) keep = m_modelList->currentItem()->data(Qt::UserRole).toString();
        const QString act = MC()->activeId(); m_modelList->blockSignals(true); m_modelList->clear(); int sel = 0, i = 0;
        for (const ModelEntry &e : MC()->all()) {
            auto *it = new QListWidgetItem(e.name + (e.id == act ? QStringLiteral("   ★ etkin") : QString()) + "\n" + e.family + " · " + e.location()); it->setData(Qt::UserRole, e.id);
            m_modelList->addItem(it); if (e.id == keep) sel = i; ++i;
        }
        m_modelList->setCurrentRow(sel); m_modelList->blockSignals(false); showModel();
    }
    void showModel() {
        auto *it = m_modelList->currentItem(); if (!it) return;
        const ModelEntry e = MC()->byId(it->data(Qt::UserRole).toString()); m_modelDetail->setText(modelDetailHtml(e));
        const bool custom = !e.builtin && !e.id.startsWith("cfg:"); m_mEdit->setEnabled(custom); m_mDel->setEnabled(custom); m_mAct->setEnabled(e.id != MC()->activeId());
    }
    // ---- 5) API ve Bağlantılar ----
    void buildApi() {
        auto *l = mkPage(QStringLiteral("API ve Bağlantılar"), QStringLiteral("Sağlayıcıları yapılandırın ve bağlantıyı gerçek bir istekle test edin. API anahtarları Windows Kimlik Bilgisi Yöneticisi'nde saklanır; günlüklere yazılmaz."));
        auto *seg = new QWidget; auto *sh = hb(seg, 0, 6); auto *sg = new QButtonGroup(this); sg->setExclusive(true);
        FlatButton *b1 = fb("key", QStringLiteral("Bulut / API sağlayıcıları"))->kind(FlatButton::Outline)->strong()->isz(18)->fixed(230, 38), *b2 = fb("server", QStringLiteral("RÜZGAR Local Server"))->kind(FlatButton::Outline)->strong()->isz(18)->fixed(210, 38);
        b1->setCheckable(true); b2->setCheckable(true); b1->setChecked(true); sg->addButton(b1, 0); sg->addButton(b2, 1); sh->addWidget(b1); sh->addWidget(b2); sh->addStretch(1);
        m_apiStack = new QStackedWidget; m_apiStack->addWidget(buildCloud()); m_apiStack->addWidget(buildLocal());
        connect(sg, &QButtonGroup::idClicked, this, [this](int i) { m_apiStack->setCurrentIndex(i); fadeWidget(m_apiStack->currentWidget(), true, 160); });
        l->addWidget(seg); l->addSpacing(6); l->addWidget(m_apiStack); l->addStretch(1);
    }
    QWidget *buildCloud() {
        auto *w = new QWidget; auto *h = hb(w, 0, 22);
        auto *left = new QWidget; left->setFixedWidth(250); auto *lv = vb(left, 0, 8);
        m_provList = new QListWidget; m_provList->setMinimumHeight(420);
        auto *addBtn = fb("plus", QStringLiteral("Sağlayıcı ekle"), left)->kind(FlatButton::Outline)->strong()->isz(18)->fixed(250, 38);
        lv->addWidget(m_provList, 1); lv->addWidget(addBtn);
        auto *form = new QWidget; auto *g = new QGridLayout(form); g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(14); g->setVerticalSpacing(10); g->setColumnStretch(1, 1); int r = 0;
        auto add = [&](const QString &lbl, QWidget *f, QLabel **keepLbl = nullptr) { auto *ll = mkLabel(lbl, "muted"); g->addWidget(ll, r, 0); g->addWidget(f, r, 1); if (keepLbl) *keepLbl = ll; ++r; };
        m_pName = textField(nullptr, QStringLiteral("Görünen ad")); add(QStringLiteral("Sağlayıcı adı"), m_pName);
        m_pKind = new QComboBox; for (PK k : {PK::OpenAI, PK::Anthropic, PK::DeepSeek, PK::Gemini, PK::Compat, PK::Custom}) m_pKind->addItem(pkName(k), int(k)); add(QStringLiteral("Sağlayıcı türü"), m_pKind);
        m_pUrl = textField(nullptr, QStringLiteral("https://…/v1")); add(QStringLiteral("API Base URL"), m_pUrl);
        m_pKey = pwField(nullptr, QStringLiteral("API anahtarı")); add(QStringLiteral("API anahtarı"), m_pKey);
        m_pModel = new QComboBox; m_pModel->setEditable(true); m_pModel->setInsertPolicy(QComboBox::NoInsert); add(QStringLiteral("Model kimliği"), m_pModel);
        m_pOrg = textField(nullptr, QStringLiteral("İsteğe bağlı")); add(QStringLiteral("Kuruluş kimliği"), m_pOrg, &m_pOrgLbl);
        m_pTimeout = numSpin(5, 900, 60, 110); add(QStringLiteral("İstek zaman aşımı (sn)"), m_pTimeout);
        m_pVision = new QComboBox; m_pVision->addItems({QStringLiteral("Otomatik"), QStringLiteral("Evet (görsel destekli)"), QStringLiteral("Hayır")}); add(QStringLiteral("Görsel desteği"), m_pVision);
        m_pHint = mkLabel(QString(), "small"); m_pHint->setWordWrap(true); g->addWidget(m_pHint, r++, 1);
        auto *br = new QWidget; auto *bh = hb(br, 0, 8);
        auto *save = pbtn(QStringLiteral("Kaydet"), "primary"); m_pTest = pbtn(QStringLiteral("Bağlantıyı test et")); m_pDef = pbtn(QStringLiteral("Varsayılan yap")); m_pDel = pbtn(QStringLiteral("Sil"), "danger");
        bh->addWidget(save); bh->addWidget(m_pTest); bh->addWidget(m_pDef); bh->addWidget(m_pDel); bh->addStretch(1); g->addWidget(br, r++, 1);
        m_pStatus2 = new QLabel; setStatus(m_pStatus2, QStringLiteral("Durum: test edilmedi"), 0); g->addWidget(m_pStatus2, r++, 1);
        h->addWidget(left); h->addWidget(form, 1);
        auto *menuBtn = addBtn;
        connect(menuBtn, &QAbstractButton::clicked, this, [this, menuBtn] {
            QMenu m(this); QList<QAction *> acts;
            for (PK k : {PK::OpenAI, PK::Anthropic, PK::DeepSeek, PK::Gemini, PK::Compat, PK::Custom}) { QAction *a = m.addAction(pkName(k)); a->setData(int(k)); acts << a; }
            QAction *r2 = m.exec(menuBtn->mapToGlobal(QPoint(0, menuBtn->height() + 4))); if (r2) newProvider(PK(r2->data().toInt()));
        });
        connect(m_pKind, &QComboBox::currentIndexChanged, this, [this] { onKindChanged(); });
        connect(m_provList, &QListWidget::currentRowChanged, this, [this](int row) { if (m_loading || row < 0) return; selectProvider(m_provList->item(row)->data(Qt::UserRole).toString()); });
        connect(save, &QPushButton::clicked, this, [this] { saveProvider(); });
        connect(m_pTest, &QPushButton::clicked, this, [this] {
            const ProviderCfg c = formCfg(); m_testId = c.id; m_pTest->setEnabled(false); setStatus(m_pStatus2, QStringLiteral("Test ediliyor…"), 0); AM()->test(c, m_pKey->text());
        });
        connect(AM(), &ApiManager::testDone, this, [this](const QString &id, bool ok, const QString &msg, const QStringList &models) {
            if (id != m_testId) return; m_pTest->setEnabled(true); setStatus(m_pStatus2, QStringLiteral("Durum: ") + msg, ok ? 1 : 2);
            if (ok && !models.isEmpty()) { const QString cur = m_pModel->currentText(); m_pModel->clear(); m_pModel->addItems(models); m_pModel->setEditText(cur.isEmpty() ? models.first() : cur); }
        });
        connect(m_pDef, &QPushButton::clicked, this, [this] { if (!m_curId.isEmpty()) AM()->setDefault(m_curId); });
        connect(m_pDel, &QPushButton::clicked, this, [this] {
            if (m_curId.isEmpty()) { newProvider(PK::OpenAI); return; }
            if (confirmBox(this, QStringLiteral("Sağlayıcıyı sil"), QStringLiteral("“%1” yapılandırması ve kayıtlı API anahtarı silinsin mi?").arg(m_pName->text()))) { AM()->remove(m_curId); m_curId.clear(); refreshProviders(); }
        });
        connect(AM(), &ApiManager::changed, this, [this] { refreshProviders(false); });
        m_lastKind = PK::OpenAI; refreshProviders();
        return w;
    }
    void refreshProviders(bool reselect = true) {
        m_loading = true; m_provList->clear(); int sel = -1, i = 0;
        for (const ProviderCfg &c : AM()->providers()) {
            auto *it = new QListWidgetItem(c.name + "\n" + pkName(c.kind) + (c.id == AM()->defaultId() ? QStringLiteral(" · varsayılan") : QString())); it->setData(Qt::UserRole, c.id); m_provList->addItem(it);
            if (c.id == m_curId) sel = i; ++i;
        }
        if (sel >= 0) m_provList->setCurrentRow(sel);
        m_loading = false;
        if (reselect) { if (sel >= 0) selectProvider(m_curId); else if (m_provList->count() > 0) { m_provList->setCurrentRow(0); selectProvider(m_provList->item(0)->data(Qt::UserRole).toString()); } else newProvider(PK::OpenAI); }
        m_pDef->setEnabled(!m_curId.isEmpty() && AM()->defaultId() != m_curId);
    }
    void loadForm(const ProviderCfg &c, const QString &key) {
        m_loading = true; m_pName->setText(c.name); m_pKind->setCurrentIndex(qMax(0, m_pKind->findData(int(c.kind)))); m_lastKind = c.kind; m_pUrl->setText(c.baseUrl); m_pKey->setText(key); m_storedKey = key;
        m_pModel->clear(); m_pModel->setEditText(c.model); m_pOrg->setText(c.org); m_pTimeout->setValue(c.timeout); m_pVision->setCurrentIndex(c.vision < 0 ? 0 : (c.vision == 1 ? 1 : 2));
        m_loading = false; updateKindUi();
    }
    void selectProvider(const QString &id) {
        bool ok = false; const ProviderCfg c = AM()->find(id, &ok); if (!ok) return;
        m_curId = id; loadForm(c, AM()->key(id)); m_pDef->setEnabled(AM()->defaultId() != id);
        if (c.lastTested > 0) setStatus(m_pStatus2, QStringLiteral("Son test (%1): %2").arg(QDateTime::fromMSecsSinceEpoch(c.lastTested).toString("dd.MM.yyyy HH:mm"), c.lastMsg), c.lastOk ? 1 : 2);
        else setStatus(m_pStatus2, QStringLiteral("Durum: test edilmedi"), 0);
    }
    void newProvider(PK k) {
        m_curId.clear(); ProviderCfg c; c.kind = k; c.name = pkName(k); c.baseUrl = pkDefaultUrl(k); c.model = pkDefaultModel(k); c.timeout = 60;
        loadForm(c, QString()); m_provList->clearSelection(); m_pDef->setEnabled(false); setStatus(m_pStatus2, QStringLiteral("Yeni yapılandırma — kaydetmeyi unutmayın."), 0);
    }
    ProviderCfg formCfg() const {
        ProviderCfg c; c.id = m_curId; c.name = m_pName->text().trimmed(); c.kind = PK(m_pKind->currentData().toInt()); c.baseUrl = m_pUrl->text().trimmed(); c.model = m_pModel->currentText().trimmed();
        c.org = m_pOrg->text().trimmed(); c.timeout = m_pTimeout->value(); c.vision = m_pVision->currentIndex() == 0 ? -1 : (m_pVision->currentIndex() == 1 ? 1 : 0); return c;
    }
    void saveProvider() {
        ProviderCfg c = formCfg(); if (c.id.isEmpty()) c.id = newId();
        const QString key = m_pKey->text(); const QString e = AM()->save(c, key, key != m_storedKey);
        if (!e.isEmpty()) { setStatus(m_pStatus2, e, 2); return; }
        m_curId = c.id; m_storedKey = key; setStatus(m_pStatus2, QStringLiteral("Kaydedildi."), 1); refreshProviders(false);
        for (int i = 0; i < m_provList->count(); ++i) if (m_provList->item(i)->data(Qt::UserRole).toString() == m_curId) { m_loading = true; m_provList->setCurrentRow(i); m_loading = false; }
    }
    void onKindChanged() {
        if (m_loading) return; const PK nk = PK(m_pKind->currentData().toInt());
        if (m_pUrl->text().trimmed().isEmpty() || m_pUrl->text().trimmed() == pkDefaultUrl(m_lastKind)) m_pUrl->setText(pkDefaultUrl(nk));
        if (m_pModel->currentText().trimmed().isEmpty() || m_pModel->currentText().trimmed() == pkDefaultModel(m_lastKind)) m_pModel->setEditText(pkDefaultModel(nk));
        if (m_pName->text().trimmed().isEmpty() || m_pName->text().trimmed() == pkName(m_lastKind)) m_pName->setText(pkName(nk));
        m_lastKind = nk; updateKindUi();
    }
    void updateKindUi() {
        const PK k = PK(m_pKind->currentData().toInt()); const bool org = (k == PK::OpenAI || k == PK::Compat);
        m_pOrg->setVisible(org); m_pOrgLbl->setVisible(org);
        QString h;
        switch (k) {
        case PK::OpenAI: h = QStringLiteral("OpenAI Chat Completions uç noktası. Düşünme modları yalnızca desteklenen akıl yürütme modellerinde (reasoning_effort) iletilir."); break;
        case PK::Anthropic: h = QStringLiteral("Anthropic Messages API (x-api-key). Düşünme modları, Claude modellerinde genişletilmiş düşünme bütçesi olarak iletilir."); break;
        case PK::DeepSeek: h = QStringLiteral("DeepSeek, OpenAI uyumlu sohbet uç noktası kullanır. Görsel girdisi varsayılan olarak desteklenmiyor kabul edilir."); break;
        case PK::Gemini: h = QStringLiteral("Google Gemini generateContent (akışlı) uç noktası. Düşünme bütçesi yalnızca Gemini 2.5+ modellerinde iletilir."); break;
        case PK::Compat: h = QStringLiteral("OpenAI uyumlu herhangi bir sunucu (LM Studio, vLLM, OpenRouter vb.). Görsel desteği bilinmediği için istek sunucuya gönderilir; sunucu reddederse hata gösterilir."); break;
        default: h = QStringLiteral("Özel uç nokta, OpenAI sohbet biçimini (chat/completions) kullanmalıdır. /models uç noktası yoksa bağlantı testi hata verebilir."); break;
        }
        m_pHint->setText(h + QStringLiteral(" Model kimliğini sağlayıcının güncel belgelerinden doğrulayın."));
    }
    QWidget *buildLocal() {
        auto *w = new QWidget; auto *g = new QGridLayout(w); g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(14); g->setVerticalSpacing(10); g->setColumnStretch(1, 1); int r = 0;
        auto info = mkLabel(QStringLiteral("Bu bağlantı bulut sağlayıcılarından ayrıdır. Sunucu OpenAI uyumlu /v1/chat/completions ve /v1/models uç noktalarını sunmalıdır (örn. gelecekteki RÜZGAR Python sunucusu)."), "muted"); info->setWordWrap(true); g->addWidget(info, r++, 0, 1, 2);
        auto add = [&](const QString &lbl, QWidget *f) { g->addWidget(mkLabel(lbl, "muted"), r, 0); g->addWidget(f, r, 1); ++r; };
        m_lAddr = textField(nullptr, QStringLiteral("http://127.0.0.1"), LS()->address()); add(QStringLiteral("Sunucu adresi"), m_lAddr);
        m_lPort = numSpin(1, 65535, LS()->port(), 110); add(QStringLiteral("Bağlantı noktası"), m_lPort);
        m_lKey = pwField(nullptr, QStringLiteral("İsteğe bağlı API anahtarı")); m_lKey->setText(LS()->key()); m_lStoredKey = LS()->key(); add(QStringLiteral("API anahtarı"), m_lKey);
        m_lModel = new QComboBox; m_lModel->setEditable(true); m_lModel->setInsertPolicy(QComboBox::NoInsert); m_lModel->addItems(LS()->models()); m_lModel->setEditText(LS()->model()); add(QStringLiteral("Seçili model"), m_lModel);
        m_lModels = mkLabel(QString(), "small"); add(QStringLiteral("Kullanılabilir modeller"), m_lModels);
        auto *br = new QWidget; auto *bh = hb(br, 0, 8); auto *save = pbtn(QStringLiteral("Kaydet"), "primary"); m_lTest = pbtn(QStringLiteral("Bağlantıyı test et")); bh->addWidget(save); bh->addWidget(m_lTest); bh->addStretch(1); g->addWidget(br, r++, 1);
        m_lStatus = new QLabel; g->addWidget(m_lStatus, r++, 1); g->setRowStretch(r, 1);
        updLocal();
        connect(save, &QPushButton::clicked, this, [this] {
            const QString e = LS()->save(m_lAddr->text(), m_lPort->value(), m_lKey->text(), m_lModel->currentText());
            if (e.isEmpty()) { m_lStoredKey = m_lKey->text(); setStatus(m_lStatus, QStringLiteral("Kaydedildi."), 1); } else setStatus(m_lStatus, e, 2);
        });
        connect(m_lTest, &QPushButton::clicked, this, [this] { m_lTest->setEnabled(false); setStatus(m_lStatus, QStringLiteral("Test ediliyor…"), 0); LS()->test(m_lAddr->text(), m_lPort->value(), m_lKey->text()); });
        connect(LS(), &LocalServerConnection::testDone, this, [this](bool ok, const QString &msg, const QStringList &models) {
            m_lTest->setEnabled(true); setStatus(m_lStatus, QStringLiteral("Sunucu durumu: ") + (ok ? QStringLiteral("Bağlı") : QStringLiteral("Bağlı değil")) + "\n" + msg, ok ? 1 : 2);
            if (ok) { const QString cur = m_lModel->currentText(); m_lModel->clear(); m_lModel->addItems(models); m_lModel->setEditText(cur.isEmpty() && !models.isEmpty() ? models.first() : cur); }
            updLocal();
        });
        return w;
    }
    void updLocal() { m_lModels->setText(LS()->models().isEmpty() ? QStringLiteral("Henüz listelenmedi (bağlantıyı test edin)") : QStringLiteral("%1 model: ").arg(LS()->models().size()) + LS()->models().mid(0, 6).join(", ") + (LS()->models().size() > 6 ? " …" : "")); }
    // ---- 6) Donanım ----
    void buildHardware() {
        auto *l = mkPage(QStringLiteral("Donanım"), QStringLiteral("Canlı CPU, RAM ve (NVIDIA için) VRAM bilgisi. Okunamayan değerler uydurulmaz."));
        l->addSpacing(8); l->addWidget(new HardwarePanel(false)); l->addStretch(1);
    }
    // ---- 7) Sohbet ----
    void buildChat() {
        auto *l = mkPage(QStringLiteral("Sohbet"), QStringLiteral("Mesaj gönderme ve sohbet geçmişi ayarları."));
        auto *en = new QComboBox; en->addItems({QStringLiteral("Enter ile gönder (Shift+Enter: yeni satır)"), QStringLiteral("Ctrl+Enter ile gönder (Enter: yeni satır)")}); en->setFixedWidth(330); en->setCurrentIndex(Cfg::get("enterSends", true).toBool() ? 0 : 1);
        connect(en, &QComboBox::currentIndexChanged, this, [](int i) { Cfg::set("enterSends", i == 0); });
        l->addWidget(settingRow(QStringLiteral("Enter davranışı"), QString(), en)); l->addWidget(hline());
        auto *sc = new QCheckBox; sc->setChecked(Cfg::get("streamUi", true).toBool()); connect(sc, &QCheckBox::toggled, this, [](bool b) { Cfg::set("streamUi", b); });
        l->addWidget(settingRow(QStringLiteral("Yanıtı akış halinde göster"), QStringLiteral("Kapalıyken yanıt tamamlanana kadar yazıyor göstergesi görünür, sonra metin bir kerede gösterilir."), sc)); l->addWidget(hline());
        auto *hs = numSpin(0, 500, Cfg::get("maxHist", 0).toInt()); connect(hs, &QSpinBox::valueChanged, this, [](int v) { Cfg::set("maxHist", v); });
        l->addWidget(settingRow(QStringLiteral("Gönderilecek son mesaj sayısı"), QStringLiteral("0 = tüm sohbet geçmişi gönderilir. Düşük değerler maliyeti ve gecikmeyi azaltır."), hs)); l->addWidget(hline());
        auto *mt = numSpin(256, 64000, Cfg::get("maxTokens", 4096).toInt()); connect(mt, &QSpinBox::valueChanged, this, [](int v) { Cfg::set("maxTokens", v); });
        l->addWidget(settingRow(QStringLiteral("Anthropic için en fazla yanıt token'ı"), QStringLiteral("Anthropic Messages API bu değeri zorunlu tutar; diğer sağlayıcılara gönderilmez."), mt)); l->addWidget(hline());
        auto *ex = pbtn(QStringLiteral("JSON olarak dışa aktar…")); auto *del = pbtn(QStringLiteral("Tüm sohbetleri sil"), "danger");
        connect(ex, &QPushButton::clicked, this, [this] {
            const QString p = QFileDialog::getSaveFileName(this, QStringLiteral("Sohbetleri dışa aktar"), QDir::homePath() + "/ruzgar-sohbetler.json", QStringLiteral("JSON (*.json)")); if (p.isEmpty()) return;
            QFile f(p); if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { QMessageBox::warning(this, QStringLiteral("Hata"), QStringLiteral("Dosya yazılamadı: ") + f.errorString()); return; }
            f.write(QJsonDocument(CM()->exportAll()).toJson(QJsonDocument::Indented)); f.close();
        });
        connect(del, &QPushButton::clicked, this, [this] { if (confirmBox(this, QStringLiteral("Tüm sohbetleri sil"), QStringLiteral("Bu hesaptaki TÜM sohbetler kalıcı olarak silinsin mi?"))) { CM()->deleteAll(); m_cleared = true; } });
        l->addWidget(settingRow(QStringLiteral("Sohbetleri dışa aktar"), QStringLiteral("Tüm sohbetlerinizi tek bir JSON dosyasına kaydeder."), ex)); l->addWidget(hline());
        l->addWidget(settingRow(QStringLiteral("Tüm sohbetleri sil"), QStringLiteral("Bu işlem geri alınamaz."), del)); l->addStretch(1);
    }
    // ---- 8) Hesap ----
    void buildAccount() {
        auto *l = mkPage(QStringLiteral("Hesap"), QStringLiteral("Bu bir yerel hesaptır: yalnızca bu bilgisayarda saklanır, cihazlar arasında eşitlenmez."));
        const UserRec u = UM()->user();
        m_aUser = textField(nullptr, QStringLiteral("Kullanıcı adı"), u.username); m_aMail = textField(nullptr, QStringLiteral("E-posta"), u.email); m_aUser->setFixedWidth(280); m_aMail->setFixedWidth(280);
        l->addWidget(settingRow(QStringLiteral("Kullanıcı adı"), QString(), m_aUser)); l->addWidget(hline());
        l->addWidget(settingRow(QStringLiteral("E-posta"), QString(), m_aMail));
        m_aStatus = new QLabel; auto *sp = pbtn(QStringLiteral("Profili kaydet"), "primary"); sp->setFixedWidth(150);
        connect(sp, &QPushButton::clicked, this, [this] {
            const UserRec cu = UM()->user(); const QString e = UM()->updateProfile(cu.first, cu.last, m_aUser->text(), m_aMail->text(), cu.initials == UserManager::initialsOf(cu.first, cu.last) ? QString() : cu.initials);
            setStatus(m_aStatus, e.isEmpty() ? QStringLiteral("Profil kaydedildi.") : e, e.isEmpty() ? 1 : 2);
        });
        l->addWidget(sp); l->addWidget(m_aStatus); l->addSpacing(10); l->addWidget(hline());
        l->addWidget(mkLabel(QStringLiteral("Parolayı değiştir"), "h2"));
        auto *o = pwField(nullptr, QStringLiteral("Mevcut parola")), *n1 = pwField(nullptr, QStringLiteral("Yeni parola (en az 8 karakter)")), *n2 = pwField(nullptr, QStringLiteral("Yeni parolayı doğrula"));
        for (QLineEdit *e : {o, n1, n2}) e->setMaximumWidth(380);
        auto *pst = new QLabel; auto *cp = pbtn(QStringLiteral("Parolayı değiştir")); cp->setFixedWidth(170);
        connect(cp, &QPushButton::clicked, this, [=] { const QString e = UM()->changePassword(o->text(), n1->text(), n2->text()); setStatus(pst, e.isEmpty() ? QStringLiteral("Parola değiştirildi.") : e, e.isEmpty() ? 1 : 2); if (e.isEmpty()) { o->clear(); n1->clear(); n2->clear(); } });
        l->addWidget(o); l->addWidget(n1); l->addWidget(n2); l->addWidget(cp); l->addWidget(pst); l->addSpacing(10); l->addWidget(hline());
        auto *so = pbtn(QStringLiteral("Çıkış yap")); auto *da = pbtn(QStringLiteral("Yerel hesabı sil"), "danger");
        connect(so, &QPushButton::clicked, this, [this] { m_signOut = true; accept(); });
        connect(da, &QPushButton::clicked, this, [this] {
            QInputDialog d(this); d.setWindowTitle(QStringLiteral("Hesabı sil")); d.setLabelText(QStringLiteral("Hesabınız ve tüm sohbetleriniz kalıcı olarak silinecek. Onaylamak için parolanızı girin:"));
            d.setTextEchoMode(QLineEdit::Password); d.setOkButtonText(QStringLiteral("Hesabı sil")); d.setCancelButtonText(QStringLiteral("İptal")); d.setMinimumWidth(420);
            if (d.exec() != QDialog::Accepted) return;
            const QString e = UM()->deleteAccount(d.textValue());
            if (!e.isEmpty()) { QMessageBox::warning(this, QStringLiteral("Hesap silinemedi"), e); return; }
            m_signOut = true; accept();
        });
        l->addWidget(settingRow(QStringLiteral("Çıkış yap"), QStringLiteral("Oturumu kapatır; hesabınız ve sohbetleriniz bu bilgisayarda kalır."), so)); l->addWidget(hline());
        l->addWidget(settingRow(QStringLiteral("Yerel hesabı sil"), QStringLiteral("Hesabı ve tüm sohbetleri kalıcı olarak siler. API anahtarları ayrıca silinmez."), da)); l->addStretch(1);
    }
    // ---- 9) Hakkında ----
    void buildAbout() {
        auto *l = mkPage(QStringLiteral("RÜZGAR AI Hakkında"), QString());
        auto *top = new QWidget; auto *th = hb(top, 0, 14); th->addWidget(new LogoLabel(56)); auto *tv = new QVBoxLayout; tv->addWidget(mkLabel(QStringLiteral("RÜZGAR AI"), "h2")); tv->addWidget(mkLabel(QStringLiteral("Sürüm 1.0 · Qt %1").arg(QString::fromLatin1(qVersion())), "muted")); th->addLayout(tv); th->addStretch(1); l->addWidget(top); l->addSpacing(8);
        const QString txt = QStringLiteral(
            "• Hesaplar ve sohbetler yerel SQLite veritabanında saklanır: %1\n"
            "• Parolalar tuzlanmış PBKDF2-HMAC-SHA256 ile özetlenir; düz metin saklanmaz.\n"
            "• API anahtarları Windows Kimlik Bilgisi Yöneticisi'nde tutulur.\n"
            "• Hesaplar cihazlar arasında eşitlenmez.\n"
            "• Ses tanıma bu sürümde yoktur (mikrofon düğmesi yalnızca arayüz öğesidir).\n"
            "• GPU/VRAM bilgisi yalnızca NVIDIA (nvidia-smi) için okunur.\n"
            "• Model bellek değerleri tahmindir; gerçek ölçüm değildir.\n"
            "• Yapay zekâ yanıtlarındaki kod hiçbir zaman çalıştırılmaz.").arg(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));
        auto *t = new QLabel(txt); t->setWordWrap(true); t->setTextInteractionFlags(Qt::TextSelectableByMouse); l->addWidget(t); l->addStretch(1);
    }
    QStackedWidget *m_stack; QButtonGroup *m_grp; bool m_signOut = false, m_cleared = false, m_shown = false, m_loading = false;
    AvatarWidget *m_avatar; QLineEdit *m_pFirst, *m_pLast, *m_pIni; QTextEdit *m_pIns; QLabel *m_pStatus;
    QListWidget *m_modelList; QLabel *m_modelDetail; QPushButton *m_mAct, *m_mEdit, *m_mDel;
    QStackedWidget *m_apiStack; QListWidget *m_provList; QLineEdit *m_pName, *m_pUrl, *m_pKey, *m_pOrg; QComboBox *m_pKind, *m_pModel, *m_pVision; QSpinBox *m_pTimeout; QLabel *m_pHint, *m_pStatus2, *m_pOrgLbl = nullptr;
    QPushButton *m_pTest, *m_pDef, *m_pDel; QString m_curId, m_storedKey, m_testId; PK m_lastKind = PK::OpenAI;
    QLineEdit *m_lAddr, *m_lKey; QSpinBox *m_lPort; QComboBox *m_lModel; QLabel *m_lModels, *m_lStatus; QPushButton *m_lTest; QString m_lStoredKey;
    QLineEdit *m_aUser, *m_aMail; QLabel *m_aStatus;
};

// ============================================================================
// 12) Görseller / bilgi sayfaları, ana pencere, giriş noktası
// ============================================================================
class ImagesPage : public QWidget {
    Q_OBJECT
public:
    explicit ImagesPage(QWidget *p = nullptr) : QWidget(p) {
        auto *v = vb(this);
        auto *hd = mkLabel(QStringLiteral("Görseller"), "h1", this); hd->setContentsMargins(36, 22, 0, 4);
        auto *sub = mkLabel(QStringLiteral("Sohbetlerinize eklediğiniz görseller. Açmak için tıklayın."), "muted", this); sub->setContentsMargins(36, 0, 0, 10);
        m_scroll = new SmoothScroll(this); m_host = new QWidget; m_grid = new QGridLayout(m_host); m_grid->setContentsMargins(36, 6, 36, 30); m_grid->setSpacing(12); m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_scroll->setWidget(m_host);
        m_empty = mkLabel(QStringLiteral("Henüz görsel eklemediniz. Mesaj alanındaki + düğmesiyle eklediğiniz görseller burada görünür."), "muted", this); m_empty->setAlignment(Qt::AlignCenter); m_empty->setWordWrap(true);
        v->addWidget(hd); v->addWidget(sub); v->addWidget(m_scroll, 1); v->addWidget(m_empty, 1);
    }
    void reload() { m_items = CM()->allImages(); rebuild(); }
protected:
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e); const int c = cols(); if (c != m_cols) rebuild(); }
private:
    int cols() const { return qMax(1, (width() - 72 + 12) / (160 + 12)); }
    void rebuild() {
        while (QLayoutItem *it = m_grid->takeAt(0)) { if (it->widget()) it->widget()->deleteLater(); delete it; }
        m_cols = cols(); int i = 0;
        for (const auto &pr : m_items) {
            const QString path = pr.first; const bool ex = QFileInfo::exists(path);
            auto *ri = new RoundImage(ex ? thumbPix(path, 160) : QPixmap(), QSize(160, 160), 14, ex ? QFileInfo(path).fileName() : QStringLiteral("Dosya bulunamadı"), m_host);
            if (ex) { ri->setClickable(true); ri->setToolTip(QFileInfo(path).fileName()); connect(ri, &RoundImage::clicked, this, [path] { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); }); }
            m_grid->addWidget(ri, i / m_cols, i % m_cols); ++i;
        }
        m_scroll->setVisible(!m_items.isEmpty()); m_empty->setVisible(m_items.isEmpty());
    }
    SmoothScroll *m_scroll; QWidget *m_host; QGridLayout *m_grid; QLabel *m_empty; QList<QPair<QString, QString>> m_items; int m_cols = 0;
};

class InfoPage : public QWidget {
    Q_OBJECT
public:
    explicit InfoPage(QWidget *p = nullptr) : QWidget(p) {
        auto *v = vb(this); v->setContentsMargins(36, 22, 36, 30);
        m_title = mkLabel(QString(), "h1", this); m_text = mkLabel(QString(), "muted", this); m_text->setWordWrap(true);
        v->addWidget(m_title); v->addSpacing(8); v->addWidget(m_text); v->addStretch(1);
    }
    void setContent(const QString &t, const QString &x) { m_title->setText(t); m_text->setText(x); }
private:
    QLabel *m_title, *m_text;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        setWindowTitle(QStringLiteral("RÜZGAR AI")); resize(1365, 768); setMinimumSize(900, 600);
        restoreGeometry(Cfg::get("geometry").toByteArray());
        m_root = new QStackedWidget(this); setCentralWidget(m_root);
        m_auth = new AuthScreen; m_shell = new QWidget; m_root->addWidget(m_auth); m_root->addWidget(m_shell);
        auto *h = hb(m_shell);
        m_sb = new Sidebar(m_shell); m_pages = new QStackedWidget(m_shell);
        m_chat = new ChatWidget; m_images = new ImagesPage; m_info = new InfoPage;
        m_pages->addWidget(m_chat); m_pages->addWidget(m_images); m_pages->addWidget(m_info);
        h->addWidget(m_sb); h->addWidget(m_pages, 1);
        connect(m_auth, &AuthScreen::authenticated, this, [this] { enterApp(); });
        connect(m_sb, &Sidebar::newChat, this, [this] { m_chat->newConversation(); m_sb->setActiveConv(QString()); showChat(); });
        connect(m_sb, &Sidebar::openConv, this, [this](const QString &id) { showChat(); m_chat->openConversation(id); m_sb->setActiveConv(id); });
        connect(m_sb, &Sidebar::convDeleted, this, [this](const QString &id) { if (m_chat->currentId() == id) { m_chat->newConversation(); m_sb->setActiveConv(QString()); } });
        connect(m_sb, &Sidebar::sectionRequested, this, [this](const QString &k) { openSection(k); });
        connect(m_sb, &Sidebar::accountAction, this, [this](const QString &k) { accountAction(k); });
        connect(m_sb, &Sidebar::collapseRequested, this, [this] { toggleSidebar(); });
        connect(m_sb, &Sidebar::openChanged, this, [this](bool o) { m_chat->header()->setSidebarOpen(o); });
        connect(m_chat, &ChatWidget::conversationActivated, this, [this](const QString &id) { m_sb->setActiveConv(id); });
        connect(m_chat, &ChatWidget::openSettingsRequested, this, [this](int p) { openSettings(p); });
        connect(m_chat, &ChatWidget::toggleSidebarRequested, this, [this] { toggleSidebar(); });
        connect(m_chat->header()->modelSelector(), &ModelSelector::addRequested, this, [this] { ModelEditDialog d(ModelEntry(), true, this); if (d.exec() == QDialog::Accepted) { const QString id = MC()->addCustom(d.result()); MC()->setActive(id); } });
        connect(m_chat->header()->modelSelector(), &ModelSelector::manageRequested, this, [this] { openSettings(3); });
        connect(&ThemeBus::I(), &ThemeBus::error, this, [this](const QString &m) { toast(this, m, true); });
        m_chat->header()->setSidebarOpen(m_sb->isOpen());
        auto sc = [this](const char *seq, std::function<void()> fn) { auto *s = new QShortcut(QKeySequence(QString::fromLatin1(seq)), this); connect(s, &QShortcut::activated, this, fn); };
        sc("Ctrl+N", [this] { if (m_root->currentWidget() == m_shell) { m_chat->newConversation(); m_sb->setActiveConv(QString()); showChat(); } });
        sc("Ctrl+K", [this] { if (m_root->currentWidget() == m_shell) m_sb->focusSearch(); });
        sc("Ctrl+B", [this] { if (m_root->currentWidget() == m_shell) toggleSidebar(); });
        sc("Ctrl+,", [this] { if (m_root->currentWidget() == m_shell) openSettings(0); });
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] { if (Cfg::get("theme", 0).toInt() == 2) switchTheme(this); });
#endif
        if (UM()->tryAutoLogin()) enterApp(); else { m_auth->start(); m_root->setCurrentWidget(m_auth); }
        m_sb->setFullWidth(width() < 1100 ? 280 : 340);
    }
protected:
    void resizeEvent(QResizeEvent *e) override { QMainWindow::resizeEvent(e); if (m_sb) m_sb->setFullWidth(width() < 1100 ? 280 : 340); }
    void closeEvent(QCloseEvent *e) override { Cfg::set("geometry", saveGeometry()); m_chat->stopGeneration(); QMainWindow::closeEvent(e); }
private:
    void showChat() { m_pages->setCurrentWidget(m_chat); m_sb->setActiveSection(QString()); m_chat->focusInput(); }
    void toggleSidebar() { m_sb->setOpen(!m_sb->isOpen(), true); }
    void enterApp() {
        m_root->setCurrentWidget(m_shell); m_sb->updateUser(); m_sb->refresh();
        if (Cfg::get("startup", 0).toInt() == 1) {
            const QString id = CM()->lastId();
            if (!id.isEmpty()) { m_chat->openConversation(id); m_sb->setActiveConv(id); showChat(); fadeWidget(m_shell, true, 260); return; }
        }
        m_chat->newConversation(); m_sb->setActiveConv(QString()); showChat(); fadeWidget(m_shell, true, 260);
    }
    void signOut() {
        m_chat->stopGeneration(); if (UM()->loggedIn()) UM()->logout();
        m_pages->setCurrentWidget(m_chat); m_auth->start(); m_root->setCurrentWidget(m_auth); fadeWidget(m_auth, true, 240);
    }
    void openSection(const QString &k) {
        if (k == "images") { m_images->reload(); m_pages->setCurrentWidget(m_images); }
        else {
            if (k == "tasks") m_info->setContent(QStringLiteral("Görevler"), QStringLiteral("Zamanlanmış görevler bu yerel sürümde henüz etkin değil."));
            else if (k == "library") m_info->setContent(QStringLiteral("Kitaplık"), QStringLiteral("Kitaplık bu yerel sürümde henüz etkin değil. Sabitlediğiniz sohbetlere kenar çubuğundan ulaşabilirsiniz."));
            else m_info->setContent(QStringLiteral("Keşfet"), QStringLiteral("Keşfet bölümü bu yerel sürümde henüz etkin değil."));
            m_pages->setCurrentWidget(m_info);
        }
        m_sb->setActiveSection(k); fadeWidget(m_pages->currentWidget(), true, 200);
    }
    void accountAction(const QString &k) {
        if (k == "signout") { signOut(); return; }
        static const QHash<QString, int> pg{{"settings", 0}, {"appearance", 1}, {"models", 3}, {"api", 4}, {"hardware", 5}, {"account", 7}};
        openSettings(pg.value(k, 0));
    }
    void openSettings(int page) {
        SettingsDialog dlg(this, page); dlg.exec();
        if (dlg.signOutRequested()) { signOut(); return; }
        if (dlg.conversationsCleared()) { m_chat->newConversation(); m_sb->setActiveConv(QString()); showChat(); }
        m_sb->updateUser(); m_sb->refresh();
    }
    QStackedWidget *m_root, *m_pages; AuthScreen *m_auth; QWidget *m_shell; Sidebar *m_sb; ChatWidget *m_chat; ImagesPage *m_images; InfoPage *m_info;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("RuzgarAI"));
    QCoreApplication::setApplicationName(QStringLiteral("RuzgarAI"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("RÜZGAR AI"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QFont f(QStringLiteral("Segoe UI")); f.setPixelSize(14); app.setFont(f);
    applyTheme();
    app.setWindowIcon(QIcon(logoPix(64)));
    QString err;
    if (!Db::open(&err)) {
        QMessageBox::critical(nullptr, QStringLiteral("Veritabanı hatası"), QStringLiteral("Yerel veritabanı açılamadı. Qt SQLite sürücüsü (qsqlite) eksik olabilir; windeployqt ile dağıtın.\n\n") + err);
        return 1;
    }
    g_um = new UserManager(&app); g_cm = new ConversationManager(&app); g_am = new ApiManager(&app); g_ls = new LocalServerConnection(&app);
    g_hw = new HardwareMonitor(&app); g_mc = new ModelCatalog(&app);
    MainWindow w; w.show();
    return app.exec();
}

#include "main.moc"
