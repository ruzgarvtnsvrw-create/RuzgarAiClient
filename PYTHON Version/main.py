#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
RÜZGAR AI - PySide6 masaüstü sohbet uygulaması (TEK DOSYA: main.py)
Kurulum:   pip install PySide6 keyring
Çalıştır:  python main.py
(keyring: API anahtarlarını Windows Kimlik Bilgisi Yöneticisi'nde saklamak için; yoksa anahtarlar yalnızca oturum boyunca bellekte kalır.)
"""
import sys, os, re, json, time, uuid, math, base64, hashlib, hmac, secrets, sqlite3, subprocess, ctypes, socket
import urllib.request, urllib.error
from dataclasses import dataclass, field
from pathlib import Path

from PySide6.QtCore import *
from PySide6.QtGui import *
from PySide6.QtWidgets import *
from PySide6.QtSvg import QSvgRenderer

try:
    import keyring
except Exception:  # keyring kurulu değil
    keyring = None

# ============================================================================
# 1) Ayarlar, tema, ikonlar
# ============================================================================
APP_DIR = Path(os.environ.get("APPDATA") or (Path.home() / ".local" / "share")) / "RuzgarAI"
ATT_DIR = APP_DIR / "attachments"
ATT_DIR.mkdir(parents=True, exist_ok=True)


class Cfg:
    _f = APP_DIR / "settings.json"
    _d = {}

    @classmethod
    def load(cls):
        try:
            cls._d = json.loads(cls._f.read_text("utf-8"))
        except Exception:
            cls._d = {}

    @classmethod
    def get(cls, k, d=None):
        return cls._d.get(k, d)

    @classmethod
    def set(cls, k, v):
        cls._d[k] = v
        try:
            cls._f.write_text(json.dumps(cls._d, ensure_ascii=False, indent=1), "utf-8")
        except OSError as e:
            BUS.error.emit(f"Ayarlar kaydedilemedi: {e}")

    @classmethod
    def anim(cls):
        return bool(cls.get("anim", True))


class Bus(QObject):
    theme_changed = Signal()
    error = Signal(str)
    user_changed = Signal()
    convs_changed = Signal()


BUS = None  # main() içinde oluşturulur


@dataclass
class Theme:
    dark: bool
    bg: str; sidebar: str; composer: str; text: str; text2: str; border: str; hover: str; sel: str
    accent: str; accent_hi: str; bubble: str; code_bg: str; code_head: str; danger: str; ok: str


LIGHT = Theme(False, "#FFFFFF", "#F7F7F7", "#FFFFFF", "#202020", "#777777", "#E5E5E5", "#EEEEEE", "#E7E7E7",
              "#2D6CDF", "#2459B8", "#F4F4F4", "#FAFAFA", "#EFEFEF", "#D93025", "#1E8E3E")
DARK = Theme(True, "#212121", "#171717", "#303030", "#ECECEC", "#A1A1A1", "#3C3C3C", "#2B2B2B", "#303030",
             "#4C8DF6", "#3B79DE", "#2F2F2F", "#171717", "#2A2A2A", "#F28B82", "#81C995")
T = LIGHT


def blend(a, b, t):
    ca, cb = QColor(a), QColor(b)
    return QColor.fromRgbF(ca.redF() + (cb.redF() - ca.redF()) * t, ca.greenF() + (cb.greenF() - ca.greenF()) * t, ca.blueF() + (cb.blueF() - ca.blueF()) * t)


# ---------- Lucide benzeri SVG ikonlar (24x24) ----------
def _dots(*pts):
    return "".join(f'<circle cx="{x}" cy="{y}" r="1.5" fill="CUR" stroke="none"/>' for x, y in pts)


ICONS = {
    "plus": '<path d="M12 5v14M5 12h14"/>',
    "search": '<circle cx="11" cy="11" r="7"/><path d="m21 21-4.8-4.8"/>',
    "sidebar": '<rect x="3" y="3" width="18" height="18" rx="3"/><path d="M9 3v18"/>',
    "newchat": '<path d="M12 3.5H6.5a3 3 0 0 0-3 3v11a3 3 0 0 0 3 3h11a3 3 0 0 0 3-3V12"/><path d="M9 12.5 18 3.5l2.5 2.5-9 9-3 .5z"/>',
    "image": '<rect x="3" y="3" width="18" height="18" rx="3"/><circle cx="9" cy="9" r="1.6"/><path d="m21 15-5-5L5 21"/>',
    "clock": '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.5 2"/>',
    "library": '<rect x="3.5" y="4" width="3.5" height="16"/><rect x="9.5" y="4" width="3.5" height="16"/><path d="m16 5.5 4 13.5"/>',
    "grid": '<circle cx="7" cy="7" r="3"/><circle cx="17" cy="7" r="3"/><circle cx="7" cy="17" r="3"/><circle cx="17" cy="17" r="3"/>',
    "arrowup": '<path d="M12 19V5M5 12l7-7 7 7"/>',
    "arrowdown": '<path d="M12 5v14M5 12l7 7 7-7"/>',
    "stop": '<rect x="7" y="7" width="10" height="10" rx="2" fill="CUR"/>',
    "mic": '<rect x="9" y="2" width="6" height="12" rx="3"/><path d="M5 11a7 7 0 0 0 14 0M12 18v4"/>',
    "copy": '<rect x="9" y="9" width="11.5" height="11.5" rx="2"/><path d="M5 15H4a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1h10a1 1 0 0 1 1 1v1"/>',
    "check": '<path d="m4 12.5 5.5 5.5L20 6.5"/>',
    "thumbup": '<rect x="3" y="10.5" width="4" height="10.5" rx="1"/><path d="M7 11l4-7.5h1.5l1 1.5v4h6l1.5 1.5-1.3 8-1.5 2H7z"/>',
    "thumbdown": '<g transform="translate(0 24) scale(1 -1)"><rect x="3" y="10.5" width="4" height="10.5" rx="1"/><path d="M7 11l4-7.5h1.5l1 1.5v4h6l1.5 1.5-1.3 8-1.5 2H7z"/></g>',
    "refresh": '<path d="M20 12a8 8 0 1 1-2.3-5.7"/><path d="M20 4v5h-5"/>',
    "more": _dots((5, 12), (12, 12), (19, 12)),
    "chevdown": '<path d="m6 9 6 6 6-6"/>',
    "chevup": '<path d="m6 15 6-6 6 6"/>',
    "share": '<path d="M12 3v12M8 7l4-4 4 4M5 12v8h14v-8"/>',
    "settings": '<path d="M4 6h16M4 12h16M4 18h16"/><circle cx="9" cy="6" r="2.2"/><circle cx="15" cy="12" r="2.2"/><circle cx="8" cy="18" r="2.2"/>',
    "user": '<circle cx="12" cy="8" r="4"/><path d="M4 21a8 8 0 0 1 16 0"/>',
    "cpu": '<rect x="6" y="6" width="12" height="12" rx="2"/><rect x="9.5" y="9.5" width="5" height="5" rx="1"/><path d="M9 2v4M15 2v4M9 18v4M15 18v4M2 9h4M2 15h4M18 9h4M18 15h4"/>',
    "key": '<circle cx="8" cy="15" r="4"/><path d="m11 12 10-9M18 6l3 3"/>',
    "sun": '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>',
    "info": '<circle cx="12" cy="12" r="9"/><path d="M12 16v-4"/>' + _dots((12, 8.3)),
    "trash": '<path d="M3 6h18M5 6l1 14h12l1-14M9 6V3h6v3M10 10v6M14 10v6"/>',
    "pin": '<path d="M12 16v6M9 3h6l-.5 7 3.5 4H6l3.5-4z"/>',
    "x": '<path d="M6 6l12 12M18 6 6 18"/>',
    "eye": '<path d="M2 12q10-12 20 0-10 12-20 0z"/><circle cx="12" cy="12" r="3"/>',
    "eyeoff": '<path d="M2 12q10-12 20 0-10 12-20 0z"/><circle cx="12" cy="12" r="3"/><path d="M3 3l18 18"/>',
    "brain": '<circle cx="12" cy="12" r="9"/><path d="M12 3v18M12 9h4M12 15H8"/>',
    "folder": '<path d="M3 8.5A2.5 2.5 0 0 1 5.5 6H9l2 2h7.5A2.5 2.5 0 0 1 21 10.5v7a2.5 2.5 0 0 1-2.5 2.5h-13A2.5 2.5 0 0 1 3 17.5z"/>',
    "chat": '<rect x="3" y="4" width="18" height="13" rx="3"/><path d="M8 17v4l4-4"/>',
    "server": '<rect x="3" y="4" width="18" height="7" rx="2"/><rect x="3" y="13" width="18" height="7" rx="2"/>' + _dots((7, 7.5), (7, 16.5)),
    "logout": '<path d="M9 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h4M9 12h12M16 7l5 5-5 5"/>',
    "wind": '<path d="M2 8h9c3 0 3-4.5 0-4.5M2 12h17c3.5 0 3.5-5 0-5M2 16h10c3 0 3 4.5 0 4.5"/>',
}
_PIX = {}


def pix(name, size=20, color=None):
    color = color or T.text
    key = (name, size, color)
    if key in _PIX:
        return _PIX[key]
    body = ICONS[name].replace("CUR", color)
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="{color}" stroke-width="1.9" '
           f'stroke-linecap="round" stroke-linejoin="round">{body}</svg>')
    r = QSvgRenderer(QByteArray(svg.encode("utf-8")))
    p = QPixmap(size * 2, size * 2)
    p.fill(Qt.transparent)
    p.setDevicePixelRatio(2)
    qp = QPainter(p)
    r.render(qp, QRectF(0, 0, size, size))
    qp.end()
    _PIX[key] = p
    return p


def ico(name, size=20, color=None):
    return QIcon(pix(name, size, color))


def logo_pix(sz):
    p = QPixmap(sz * 2, sz * 2)
    p.fill(Qt.transparent)
    p.setDevicePixelRatio(2)
    qp = QPainter(p)
    qp.setRenderHint(QPainter.Antialiasing)
    qp.setPen(Qt.NoPen)
    qp.setBrush(QColor(T.accent))
    qp.drawRoundedRect(QRectF(0, 0, sz, sz), sz * .3, sz * .3)
    qp.drawPixmap(QPointF(sz * .2, sz * .2), pix("wind", int(sz * .6), "#FFFFFF"))
    qp.end()
    return p


# ---------- stil ----------
def qss():
    t = T
    return f"""
QWidget{{color:{t.text};font-family:"Segoe UI","Segoe UI Variable Text",Arial,sans-serif;font-size:14px;}}
QDialog,QMainWindow{{background:{t.bg};}}
QToolTip{{background:{t.text};color:{t.bg};border:none;padding:5px 9px;border-radius:6px;}}
QLabel{{background:transparent;}}
QLabel[role="muted"]{{color:{t.text2};}}
QLabel[role="small"]{{color:{t.text2};font-size:12px;}}
QLabel[role="h1"]{{font-size:24px;font-weight:600;}}
QLabel[role="h2"]{{font-size:16px;font-weight:600;}}
QLabel[role="brand"]{{font-size:19px;font-weight:600;}}
QLabel[role="err"]{{color:{t.danger};font-size:13px;}}
QLabel[role="msg"]{{font-size:15px;}}
QLineEdit,QComboBox,QSpinBox{{background:{t.bg};border:1px solid {t.border};border-radius:8px;padding:7px 10px;selection-background-color:{t.accent};selection-color:#FFFFFF;}}
QLineEdit:focus,QComboBox:focus,QSpinBox:focus{{border:1px solid {t.accent};}}
QComboBox::drop-down{{border:none;width:26px;}}
QComboBox::down-arrow{{image:url("{ASSET['chev']}");width:12px;height:12px;}}
QComboBox QAbstractItemView{{background:{t.bg};border:1px solid {t.border};selection-background-color:{t.hover};selection-color:{t.text};outline:0;}}
QTextEdit#composerEdit{{background:transparent;border:none;padding:0;font-size:15px;selection-background-color:{t.accent};selection-color:#FFFFFF;}}
QTextEdit#field{{background:{t.bg};border:1px solid {t.border};border-radius:8px;padding:6px 8px;}}
QPlainTextEdit#codeBody{{background:transparent;border:none;font-family:Consolas,"Cascadia Mono","Courier New",monospace;font-size:13px;}}
QFrame#codeBox{{background:{t.code_bg};border:1px solid {t.border};border-radius:10px;}}
QFrame#codeHead{{background:{t.code_head};border:none;border-top-left-radius:9px;border-top-right-radius:9px;}}
QFrame#bubble{{background:{t.bubble};border:none;border-radius:18px;}}
QFrame#authCard{{background:{t.bg};border:1px solid {t.border};border-radius:16px;}}
QFrame#errBox{{background:transparent;border:1px solid {t.danger};border-radius:10px;}}
QFrame#hline,QFrame#sbLine{{background:{t.border};border:none;}}
QPushButton{{background:{t.bg};border:1px solid {t.border};border-radius:10px;padding:8px 16px;}}
QPushButton:hover{{background:{t.hover};}}
QPushButton:disabled{{color:{t.text2};}}
QPushButton[v="primary"]{{background:{t.accent};color:#FFFFFF;border:none;}}
QPushButton[v="primary"]:hover{{background:{t.accent_hi};}}
QPushButton[v="primary"]:disabled{{background:{t.hover};color:{t.text2};}}
QPushButton[v="danger"]{{color:{t.danger};border:1px solid {t.danger};}}
QPushButton[v="ghost"]{{background:transparent;border:none;padding:6px 10px;}}
QPushButton[v="ghost"]:hover,QPushButton[v="ghost"]:checked{{background:{t.hover};}}
QPushButton[v="nav"]{{background:transparent;border:none;text-align:left;padding:8px 12px;}}
QPushButton[v="nav"]:hover{{background:{t.hover};}}
QPushButton[v="nav"]:checked{{background:{t.sel};}}
QScrollArea{{border:none;background:transparent;}}
QScrollBar:vertical{{background:transparent;width:12px;margin:2px 0;}}
QScrollBar::handle:vertical{{background:{blend(t.bg, t.text, .22).name()};border-radius:4px;min-height:40px;margin:0 2px;}}
QScrollBar::handle:vertical:hover{{background:{blend(t.bg, t.text, .38).name()};}}
QScrollBar:horizontal{{background:transparent;height:12px;margin:0 2px;}}
QScrollBar::handle:horizontal{{background:{blend(t.bg, t.text, .22).name()};border-radius:4px;min-width:40px;margin:2px 0;}}
QScrollBar::add-line,QScrollBar::sub-line{{width:0;height:0;border:none;background:none;}}
QScrollBar::add-page,QScrollBar::sub-page{{background:none;}}
QMenu{{background:{t.bg};border:1px solid {t.border};padding:6px;}}
QMenu::item{{padding:8px 30px 8px 14px;border-radius:6px;margin:1px 0;}}
QMenu::item:selected{{background:{t.hover};color:{t.text};}}
QMenu::separator{{height:1px;background:{t.border};margin:5px 8px;}}
QCheckBox{{spacing:9px;}}
QCheckBox::indicator{{width:16px;height:16px;border:1px solid {t.border};border-radius:4px;background:{t.bg};}}
QCheckBox::indicator:checked{{background:{t.accent};border:1px solid {t.accent};image:url("{ASSET['check']}");}}
QListWidget{{background:transparent;border:none;outline:0;}}
QListWidget::item{{padding:9px 10px;border-radius:8px;margin:1px 0;}}
QListWidget::item:hover{{background:{t.hover};}}
QListWidget::item:selected{{background:{t.sel};color:{t.text};}}
QProgressBar{{background:{t.hover};border:none;border-radius:4px;max-height:8px;min-height:8px;color:transparent;}}
QProgressBar::chunk{{background:{t.accent};border-radius:4px;}}
"""


ASSET = {"check": "", "chev": ""}


def system_dark():
    try:
        return QGuiApplication.styleHints().colorScheme() == Qt.ColorScheme.Dark
    except Exception:
        return False


def apply_theme():
    global T
    m = Cfg.get("theme", 0)
    T = DARK if (m == 1 or (m == 2 and system_dark())) else LIGHT
    d = APP_DIR / "assets"
    d.mkdir(exist_ok=True)
    chk = QPixmap(14, 14); chk.fill(Qt.transparent)
    p = QPainter(chk); p.setRenderHint(QPainter.Antialiasing); p.setPen(QPen(QColor("white"), 2, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
    p.drawPolyline([QPointF(3, 7.5), QPointF(6, 10.5), QPointF(11, 4)]); p.end()
    chv = QPixmap(12, 12); chv.fill(Qt.transparent)
    p = QPainter(chv); p.setRenderHint(QPainter.Antialiasing); p.setPen(QPen(QColor(T.text2), 1.6, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
    p.drawPolyline([QPointF(2.5, 4.5), QPointF(6, 8), QPointF(9.5, 4.5)]); p.end()
    chk.save(str(d / "check.png"), "PNG"); chv.save(str(d / ("chev_d.png" if T.dark else "chev_l.png")), "PNG")
    ASSET["check"] = (d / "check.png").as_posix(); ASSET["chev"] = (d / ("chev_d.png" if T.dark else "chev_l.png")).as_posix()
    pal = QPalette()
    for role, col in ((QPalette.Window, T.bg), (QPalette.WindowText, T.text), (QPalette.Base, T.bg), (QPalette.AlternateBase, T.sidebar), (QPalette.Text, T.text),
                      (QPalette.Button, T.bg), (QPalette.ButtonText, T.text), (QPalette.ToolTipBase, T.text), (QPalette.ToolTipText, T.bg),
                      (QPalette.Highlight, T.accent), (QPalette.HighlightedText, "#FFFFFF"), (QPalette.PlaceholderText, T.text2), (QPalette.Link, T.accent)):
        pal.setColor(role, QColor(col))
    app = QApplication.instance()
    app.setPalette(pal)
    app.setStyleSheet(qss())
    BUS.theme_changed.emit()


def switch_theme(host):
    ov = None
    if host is not None and Cfg.anim() and host.isVisible():
        ov = QLabel(host); ov.setPixmap(host.grab()); ov.setGeometry(host.rect()); ov.setAttribute(Qt.WA_TransparentForMouseEvents); ov.show(); ov.raise_()
    apply_theme()
    if ov:
        e = QGraphicsOpacityEffect(ov); ov.setGraphicsEffect(e)
        a = QPropertyAnimation(e, b"opacity", ov); a.setDuration(320); a.setStartValue(1.0); a.setEndValue(0.0)
        a.finished.connect(ov.deleteLater); a.start(QAbstractAnimation.DeleteWhenStopped)
        ov._a = a


# ---------- yardımcılar ----------
def now_ms():
    return int(time.time() * 1000)


def new_id():
    return uuid.uuid4().hex[:16]


def label(text="", role=None, parent=None):
    l = QLabel(text, parent)
    if role:
        l.setProperty("role", role)
    return l


def vbox(w=None, m=0, sp=0):
    l = QVBoxLayout(w); l.setContentsMargins(m, m, m, m); l.setSpacing(sp); return l


def hbox(w=None, m=0, sp=0):
    l = QHBoxLayout(w); l.setContentsMargins(m, m, m, m); l.setSpacing(sp); return l


def fmt_bytes(b):
    if b is None or b < 0:
        return "Unavailable"
    g = b / 1073741824
    return f"{g:.1f} GB" if g >= 1 else f"{b / 1048576:.0f} MB"


def fade(w, show, ms=180):
    if w is None:
        return
    if not Cfg.anim():
        w.setVisible(show); return
    old = getattr(w, "_fade", None)
    if old is not None:
        old.stop()
    e = w.graphicsEffect()
    if not isinstance(e, QGraphicsOpacityEffect):
        e = QGraphicsOpacityEffect(w); e.setOpacity(0.0 if show else 1.0); w.setGraphicsEffect(e)
    a = QPropertyAnimation(e, b"opacity", w); a.setDuration(ms); a.setStartValue(e.opacity()); a.setEndValue(1.0 if show else 0.0)
    if show:
        w.show()

    def done():
        if a.currentTime() < a.duration():
            return
        if show:
            QTimer.singleShot(0, lambda: w.setGraphicsEffect(None))
        else:
            w.hide()
    a.finished.connect(done); w._fade = a; a.start()


def toast(host, msg, err=False):
    if host is None:
        return
    host = host.window()
    l = QLabel(msg, host); l.setWordWrap(True); l.setAlignment(Qt.AlignCenter); l.setMaximumWidth(520); l.setAttribute(Qt.WA_TransparentForMouseEvents)
    bg = T.danger if err else (T.text); fg = "#FFFFFF" if err else T.bg
    l.setStyleSheet(f"background:{bg};color:{fg};border-radius:12px;padding:10px 16px;font-size:13px;")
    l.adjustSize(); l.move((host.width() - l.width()) // 2, host.height() - l.height() - 130); l.raise_(); fade(l, True, 160)
    QTimer.singleShot(4500, lambda: (fade(l, False, 220), QTimer.singleShot(300, l.deleteLater)))


def confirm(parent, title, text, yes="Sil"):
    b = QMessageBox(parent); b.setIcon(QMessageBox.Warning); b.setWindowTitle(title); b.setText(text)
    y = b.addButton(yes, QMessageBox.AcceptRole); b.addButton("İptal", QMessageBox.RejectRole); b.setDefaultButton(y); b.exec()
    return b.clickedButton() is y


def ask_text(parent, title, lab, value="", password=False):
    d = QInputDialog(parent); d.setWindowTitle(title); d.setLabelText(lab); d.setTextValue(value); d.setOkButtonText("Tamam"); d.setCancelButtonText("İptal"); d.setMinimumWidth(400)
    if password:
        d.setTextEchoMode(QLineEdit.Password)
    if d.exec() != QDialog.Accepted:
        return None
    return d.textValue().strip()


# ============================================================================
# 2) Veritabanı, hesaplar, sohbetler
# ============================================================================
DDL = """
CREATE TABLE IF NOT EXISTS users(id TEXT PRIMARY KEY, first TEXT, last TEXT, username TEXT UNIQUE COLLATE NOCASE, email TEXT UNIQUE COLLATE NOCASE,
  salt BLOB, hash BLOB, iters INTEGER, initials TEXT, instructions TEXT DEFAULT '', created INTEGER);
CREATE TABLE IF NOT EXISTS conversations(id TEXT PRIMARY KEY, user_id TEXT, title TEXT, created INTEGER, updated INTEGER, pinned INTEGER DEFAULT 0, project TEXT DEFAULT '', model TEXT DEFAULT '');
CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY AUTOINCREMENT, conv_id TEXT, role TEXT, content TEXT, ts INTEGER, images TEXT DEFAULT '[]', model TEXT DEFAULT '', feedback INTEGER DEFAULT 0);
CREATE INDEX IF NOT EXISTS idx_msg_conv ON messages(conv_id, id);
CREATE INDEX IF NOT EXISTS idx_conv_user ON conversations(user_id, updated);
"""


class Db:
    con = None

    @classmethod
    def open(cls):
        cls.con = sqlite3.connect(str(APP_DIR / "ruzgar.db"), check_same_thread=False)
        cls.con.row_factory = sqlite3.Row
        cls.con.executescript(DDL)
        cls.con.commit()

    @classmethod
    def run(cls, sql, args=()):
        try:
            cur = cls.con.execute(sql, args); cls.con.commit(); return cur
        except sqlite3.Error as e:
            BUS.error.emit(f"Veritabanı hatası: {e}"); return None

    @classmethod
    def all(cls, sql, args=()):
        cur = cls.run(sql, args)
        return cur.fetchall() if cur else []

    @classmethod
    def one(cls, sql, args=()):
        cur = cls.run(sql, args)
        return cur.fetchone() if cur else None


@dataclass
class User:
    id: str = ""; first: str = ""; last: str = ""; username: str = ""; email: str = ""; initials: str = ""; instructions: str = ""

    @property
    def full_name(self):
        return f"{self.first} {self.last}".strip()


def initials_of(f, l):
    return ((f[:1] + l[:1]).replace("i", "İ").replace("ı", "I")).upper()


class Users:
    cur = User()
    ITERS = 150000
    _fails = 0
    _lock = 0

    @staticmethod
    def _hash(pw, salt, it):
        return hashlib.pbkdf2_hmac("sha256", pw.encode("utf-8"), salt, it, 32)

    @classmethod
    def has_accounts(cls):
        r = Db.one("SELECT COUNT(*) c FROM users"); return bool(r and r["c"] > 0)

    @classmethod
    def logged_in(cls):
        return bool(cls.cur.id)

    @classmethod
    def _load(cls, uid):
        r = Db.one("SELECT id,first,last,username,email,initials,instructions FROM users WHERE id=?", (uid,))
        if r:
            cls.cur = User(r["id"], r["first"], r["last"], r["username"], r["email"], r["initials"], r["instructions"] or "")
            Cfg.set("lastUser", uid)
        BUS.user_changed.emit()

    @classmethod
    def _check_fields(cls, f, l, un, em):
        if not f or not l: return "Ad ve soyad zorunludur."
        if not re.fullmatch(r"[A-Za-z0-9_.-]{3,32}", un): return "Kullanıcı adı 3–32 karakter olmalı (harf, rakam, _ . -)."
        if not re.fullmatch(r"[^@\s]+@[^@\s]+\.[^@\s]+", em): return "Geçerli bir e-posta adresi girin."
        return ""

    @classmethod
    def register(cls, f, l, un, em, pw, pw2):
        f, l, un, em = f.strip(), l.strip(), un.strip(), em.strip()
        e = cls._check_fields(f, l, un, em)
        if e: return e
        if len(pw) < 8: return "Parola en az 8 karakter olmalı."
        if pw != pw2: return "Parolalar eşleşmiyor."
        if Db.one("SELECT 1 FROM users WHERE username=? OR email=?", (un, em)): return "Bu kullanıcı adı veya e-posta zaten kayıtlı."
        salt = os.urandom(16); uid = new_id()
        if Db.run("INSERT INTO users(id,first,last,username,email,salt,hash,iters,initials,created) VALUES(?,?,?,?,?,?,?,?,?,?)",
                  (uid, f, l, un, em, salt, cls._hash(pw, salt, cls.ITERS), cls.ITERS, initials_of(f, l), now_ms())) is None:
            return "Hesap oluşturulamadı (veritabanı hatası)."
        cls._load(uid); return ""

    @classmethod
    def login(cls, key, pw):
        if time.time() < cls._lock: return f"Çok fazla başarısız deneme. {int(cls._lock - time.time()) + 1} sn sonra tekrar deneyin."
        key = key.strip()
        if not key or not pw: return "E-posta/kullanıcı adı ve parola girin."
        r = Db.one("SELECT id,salt,hash,iters FROM users WHERE username=? OR email=?", (key, key))
        ok = False
        if r: ok = hmac.compare_digest(cls._hash(pw, r["salt"], r["iters"]), r["hash"])
        else: cls._hash(pw, os.urandom(16), cls.ITERS)
        if not ok:
            cls._fails += 1
            if cls._fails >= 5: cls._lock = time.time() + 30; cls._fails = 0
            return "E-posta/kullanıcı adı veya parola hatalı."
        cls._fails = 0; cls._load(r["id"]); return ""

    @classmethod
    def auto_login(cls):
        uid = Cfg.get("lastUser")
        if not Cfg.get("keepSignedIn", True) or not uid or not Db.one("SELECT 1 FROM users WHERE id=?", (uid,)): return False
        cls._load(uid); return True

    @classmethod
    def logout(cls):
        cls.cur = User(); Cfg.set("lastUser", ""); BUS.user_changed.emit()

    @classmethod
    def verify(cls, pw):
        r = Db.one("SELECT salt,hash,iters FROM users WHERE id=?", (cls.cur.id,))
        return bool(r and hmac.compare_digest(cls._hash(pw, r["salt"], r["iters"]), r["hash"]))

    @classmethod
    def update_profile(cls, f, l, un, em, ini):
        f, l, un, em, ini = f.strip(), l.strip(), un.strip(), em.strip(), ini.strip()
        e = cls._check_fields(f, l, un, em)
        if e: return e
        if Db.one("SELECT 1 FROM users WHERE (username=? OR email=?) AND id<>?", (un, em, cls.cur.id)): return "Bu kullanıcı adı veya e-posta başka bir hesapta kayıtlı."
        ini = initials_of(f, l) if not ini else ini[:2].upper()
        if Db.run("UPDATE users SET first=?,last=?,username=?,email=?,initials=? WHERE id=?", (f, l, un, em, ini, cls.cur.id)) is None: return "Profil kaydedilemedi."
        cls._load(cls.cur.id); return ""

    @classmethod
    def set_instructions(cls, t):
        if Db.run("UPDATE users SET instructions=? WHERE id=?", (t, cls.cur.id)) is None: return "Kaydedilemedi."
        cls.cur.instructions = t; return ""

    @classmethod
    def change_password(cls, old, n1, n2):
        if not cls.verify(old): return "Mevcut parola hatalı."
        if len(n1) < 8: return "Yeni parola en az 8 karakter olmalı."
        if n1 != n2: return "Yeni parolalar eşleşmiyor."
        salt = os.urandom(16)
        return "" if Db.run("UPDATE users SET salt=?,hash=?,iters=? WHERE id=?", (salt, cls._hash(n1, salt, cls.ITERS), cls.ITERS, cls.cur.id)) else "Parola değiştirilemedi."

    @classmethod
    def delete_account(cls, pw):
        if not cls.verify(pw): return "Parola hatalı."
        uid = cls.cur.id
        Db.run("DELETE FROM messages WHERE conv_id IN (SELECT id FROM conversations WHERE user_id=?)", (uid,))
        Db.run("DELETE FROM conversations WHERE user_id=?", (uid,))
        if Db.run("DELETE FROM users WHERE id=?", (uid,)) is None: return "Hesap silinemedi."
        cls.logout(); return ""


@dataclass
class Conv:
    id: str = ""; title: str = ""; created: int = 0; updated: int = 0; pinned: bool = False; project: str = ""; model: str = ""


@dataclass
class Msg:
    id: int = 0; conv: str = ""; role: str = ""; content: str = ""; ts: int = 0; images: list = field(default_factory=list); model: str = ""; feedback: int = 0


def _conv(r):
    return Conv(r["id"], r["title"], r["created"], r["updated"], bool(r["pinned"]), r["project"] or "", r["model"] or "")


class Convs:
    @staticmethod
    def _like(s):
        return "%" + s.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_") + "%"

    @classmethod
    def list(cls, search="", project=""):
        sql = "SELECT * FROM conversations WHERE user_id=?"; args = [Users.cur.id]
        if project: sql += " AND project=?"; args.append(project)
        if search:
            sql += " AND (title LIKE ? ESCAPE '\\' OR id IN (SELECT conv_id FROM messages WHERE content LIKE ? ESCAPE '\\'))"; args += [cls._like(search)] * 2
        return [_conv(r) for r in Db.all(sql + " ORDER BY pinned DESC, updated DESC", args)]

    @classmethod
    def get(cls, cid):
        r = Db.one("SELECT * FROM conversations WHERE id=? AND user_id=?", (cid, Users.cur.id)); return _conv(r) if r else Conv()

    @classmethod
    def last_id(cls):
        r = Db.one("SELECT id FROM conversations WHERE user_id=? ORDER BY updated DESC LIMIT 1", (Users.cur.id,)); return r["id"] if r else ""

    @classmethod
    def _chg(cls, sql, args):
        if Db.run(sql, args) is not None: BUS.convs_changed.emit()

    @classmethod
    def create(cls, title="Yeni sohbet", model=""):
        cid = new_id(); t = now_ms()
        if Db.run("INSERT INTO conversations(id,user_id,title,created,updated,pinned,project,model) VALUES(?,?,?,?,?,0,'',?)", (cid, Users.cur.id, title, t, t, model)) is None: return ""
        BUS.convs_changed.emit(); return cid

    @classmethod
    def rename(cls, cid, t): cls._chg("UPDATE conversations SET title=? WHERE id=?", (t.strip() or "Yeni sohbet", cid))
    @classmethod
    def set_pinned(cls, cid, p): cls._chg("UPDATE conversations SET pinned=? WHERE id=?", (1 if p else 0, cid))
    @classmethod
    def set_project(cls, cid, pr): cls._chg("UPDATE conversations SET project=? WHERE id=?", (pr, cid))
    @classmethod
    def rename_project(cls, o, n): cls._chg("UPDATE conversations SET project=? WHERE project=? AND user_id=?", (n, o, Users.cur.id))
    @classmethod
    def touch(cls, cid): cls._chg("UPDATE conversations SET updated=? WHERE id=?", (now_ms(), cid))

    @classmethod
    def remove(cls, cid):
        Db.run("DELETE FROM messages WHERE conv_id=?", (cid,)); cls._chg("DELETE FROM conversations WHERE id=? AND user_id=?", (cid, Users.cur.id))

    @classmethod
    def delete_all(cls):
        Db.run("DELETE FROM messages WHERE conv_id IN (SELECT id FROM conversations WHERE user_id=?)", (Users.cur.id,)); cls._chg("DELETE FROM conversations WHERE user_id=?", (Users.cur.id,))

    @classmethod
    def count(cls, project):
        r = Db.one("SELECT COUNT(*) c FROM conversations WHERE user_id=? AND project=?", (Users.cur.id, project)); return r["c"] if r else 0

    @classmethod
    def messages(cls, cid):
        out = []
        for r in Db.all("SELECT * FROM messages WHERE conv_id=? ORDER BY id", (cid,)):
            try: imgs = json.loads(r["images"] or "[]")
            except ValueError: imgs = []
            out.append(Msg(r["id"], r["conv_id"], r["role"], r["content"], r["ts"], imgs, r["model"] or "", r["feedback"]))
        return out

    @classmethod
    def add_message(cls, m):
        cur = Db.run("INSERT INTO messages(conv_id,role,content,ts,images,model,feedback) VALUES(?,?,?,?,?,?,0)", (m.conv, m.role, m.content, m.ts or now_ms(), json.dumps(m.images), m.model))
        if cur is None: return 0
        cls.touch(m.conv); return cur.lastrowid

    @classmethod
    def set_feedback(cls, mid, f): Db.run("UPDATE messages SET feedback=? WHERE id=?", (f, mid))
    @classmethod
    def delete_message(cls, mid): Db.run("DELETE FROM messages WHERE id=?", (mid,))

    @classmethod
    def all_images(cls):
        out = []
        for r in Db.all("SELECT images,conv_id FROM messages WHERE role='user' AND images<>'[]' AND conv_id IN (SELECT id FROM conversations WHERE user_id=?) ORDER BY id DESC", (Users.cur.id,)):
            try: out += [(p, r["conv_id"]) for p in json.loads(r["images"])]
            except ValueError: pass
        return out

    @classmethod
    def export_all(cls):
        return [{"id": c.id, "title": c.title, "project": c.project, "pinned": c.pinned,
                 "updated": time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime(c.updated / 1000)),
                 "messages": [{"role": m.role, "content": m.content, "model": m.model, "time": time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime(m.ts / 1000))} for m in cls.messages(c.id)]} for c in cls.list()]


# ============================================================================
# 3) Sağlayıcılar, ağ katmanı, yerel sunucu
# ============================================================================
OPENAI, ANTHROPIC, DEEPSEEK, GEMINI, COMPAT, CUSTOM, LOCAL = range(7)
PK_NAMES = ["OpenAI", "Anthropic", "DeepSeek", "Google Gemini", "OpenAI uyumlu API", "Özel API uç noktası", "RÜZGAR Local Server"]
PK_URL = {OPENAI: "https://api.openai.com/v1", ANTHROPIC: "https://api.anthropic.com/v1", DEEPSEEK: "https://api.deepseek.com/v1",
          GEMINI: "https://generativelanguage.googleapis.com/v1beta", LOCAL: "http://127.0.0.1:8765/v1"}
PK_MODEL = {OPENAI: "gpt-4o", ANTHROPIC: "claude-sonnet-5-5", DEEPSEEK: "deepseek-chat", GEMINI: "gemini-2.5-flash"}
CLOUD_KINDS = [OPENAI, ANTHROPIC, DEEPSEEK, GEMINI, COMPAT, CUSTOM]
KEY_REQUIRED = (OPENAI, ANTHROPIC, DEEPSEEK, GEMINI)


@dataclass
class Prov:
    id: str = ""; name: str = ""; kind: int = OPENAI; base_url: str = ""; model: str = ""; org: str = ""
    timeout: int = 60; vision: int = -1; last_ok: bool = False; last_msg: str = ""; last_tested: int = 0


def prov_to_dict(p): return dict(p.__dict__)
def prov_from_dict(d): return Prov(**{k: d[k] for k in Prov.__dataclass_fields__ if k in d})
def trim_url(u): return u.strip().rstrip("/")


def validate_url(u):
    if not u.strip(): return "Base URL boş olamaz."
    m = re.match(r"^(https?)://([^/\s:]+)", u.strip(), re.I)
    return "" if m else "Geçersiz sunucu adresi: http:// veya https:// ile başlayan geçerli bir URL girin."


class Secrets:
    mem = {}

    @classmethod
    def save(cls, k, v):
        cls.mem[k] = v
        if keyring is None: return not v
        try:
            if v: keyring.set_password("RuzgarAI", k, v)
            else:
                try: keyring.delete_password("RuzgarAI", k)
                except Exception: pass
            return True
        except Exception:
            return False

    @classmethod
    def load(cls, k):
        if k in cls.mem: return cls.mem[k]
        v = ""
        if keyring is not None:
            try: v = keyring.get_password("RuzgarAI", k) or ""
            except Exception: v = ""
        cls.mem[k] = v; return v


def thinking_supported(kind, model):
    m = (model or "").lower()
    if kind == OPENAI: return m.startswith(("o1", "o3", "o4", "gpt-5"))
    if kind == ANTHROPIC: return "claude" in m
    if kind == GEMINI: return "gemini-2.5" in m or "gemini-3" in m
    return False


def vision_heuristic(kind, model):
    m = (model or "").lower()
    if kind == OPENAI: return 1 if m.startswith(("gpt-4o", "gpt-4.1", "gpt-4.5", "gpt-4-turbo", "gpt-5", "o1", "o3", "o4")) else -1
    if kind == ANTHROPIC: return 1 if "claude" in m else -1
    if kind == GEMINI: return 1 if "gemini" in m else -1
    if kind == DEEPSEEK: return 0
    return -1


def encode_image(path):
    """(mime, base64) döndürür; hata olursa ValueError."""
    p = Path(path)
    if not p.exists() or not os.access(p, os.R_OK): raise ValueError(f"Görsel dosyası bulunamadı veya okunamıyor: {p.name}")
    if p.stat().st_size > 20 * 1024 * 1024: raise ValueError(f"Görsel çok büyük (en fazla 20 MB): {p.name}")
    ext = p.suffix.lower().lstrip(".")
    if ext in ("png", "jpg", "jpeg", "webp"):
        return {"png": "image/png", "webp": "image/webp"}.get(ext, "image/jpeg"), base64.b64encode(p.read_bytes()).decode()
    img = QImage(str(p))
    if img.isNull(): raise ValueError(f"Görsel biçimi okunamadı: {p.name}")
    buf = QBuffer(); buf.open(QIODevice.WriteOnly)
    if not img.save(buf, "PNG"): raise ValueError(f"Görsel dönüştürülemedi: {p.name}")
    return "image/png", base64.b64encode(bytes(buf.data())).decode()


def server_message(raw):
    raw = (raw or b"").strip()
    if not raw: return ""
    try:
        d = json.loads(raw)
    except ValueError:
        return raw[:200].decode("utf-8", "replace").strip()
    if isinstance(d, list) and d: d = d[0]
    if not isinstance(d, dict): return ""
    e = d.get("error")
    if isinstance(e, dict): return str(e.get("message", ""))
    if isinstance(e, str): return e
    return str(d.get("message") or d.get("detail") or "")


def api_error_text(http, exc, srv, prov, key):
    if key: srv = (srv or "").replace(key, "***")
    host = re.sub(r"^https?://", "", prov.base_url).split("/")[0]
    if http >= 400:
        base = {401: f"Geçersiz API anahtarı veya yetkisiz erişim (HTTP {http}). Anahtarı Ayarlar → API ve Bağlantılar bölümünden kontrol edin.",
                403: f"Geçersiz API anahtarı veya yetkisiz erişim (HTTP {http}). Anahtarı Ayarlar → API ve Bağlantılar bölümünden kontrol edin.",
                404: "Uç nokta veya model bulunamadı (HTTP 404). Base URL ve model kimliğini kontrol edin.",
                408: "Sunucu isteği zaman aşımına uğrattı (HTTP 408).",
                413: "İstek çok büyük (HTTP 413). Görsel veya mesaj boyutunu azaltın.",
                429: "İstek sınırı veya kota aşıldı (HTTP 429)."}.get(http)
        if base is None:
            if http in (400, 422): base = f"İstek reddedildi (HTTP {http}): model desteklenmiyor veya istek biçimi geçersiz olabilir."
            elif http >= 500: base = f"Sağlayıcı sunucu hatası (HTTP {http}). Daha sonra tekrar deneyin."
            else: base = f"HTTP hatası {http}."
    else:
        reason = getattr(exc, "reason", exc)
        if isinstance(exc, (socket.timeout, TimeoutError)) or isinstance(reason, (socket.timeout, TimeoutError)): base = f"Bağlantı zaman aşımına uğradı ({prov.timeout} sn)."
        elif isinstance(reason, ConnectionRefusedError): base = f"Bağlantı reddedildi ({host}). Sunucu çalışıyor mu?"
        elif isinstance(reason, socket.gaierror): base = f"Sunucu adı çözümlenemedi ({host}). Adresi ve ağ bağlantınızı kontrol edin."
        elif "SSL" in type(reason).__name__ or "CERTIFICATE" in str(reason).upper(): base = f"TLS/SSL bağlantısı kurulamadı: {reason}"
        elif isinstance(reason, (ConnectionResetError, ConnectionAbortedError)): base = "Sunucu bağlantıyı beklenmedik şekilde kapattı."
        else: base = f"Ağ kullanılamıyor veya bağlantı hatası: {reason}"
    if srv: base += "\nSunucu mesajı: " + srv[:400]
    return base


@dataclass
class ChatMsg:
    role: str; text: str; images: list = field(default_factory=list)


class ApiWorker(QThread):
    delta = Signal(str)
    done = Signal()
    failed = Signal(str)

    def __init__(self, prov, key, model, msgs, thinking=0, max_tokens=4096, system=""):
        super().__init__()
        self.p, self.key, self.model, self.thinking, self.max_tokens, self.system = prov, key, model, thinking, max_tokens, system
        self.msgs = []
        for m in msgs:  # ardışık aynı rolleri birleştir, boşları at
            if not m.text.strip() and not m.images: continue
            if self.msgs and self.msgs[-1].role == m.role:
                self.msgs[-1].text += "\n\n" + m.text; self.msgs[-1].images += m.images
            else: self.msgs.append(ChatMsg(m.role, m.text, list(m.images)))
        self._abort = False; self._resp = None; self.got = False; self.err = ""

    def abort(self):
        self._abort = True
        try:
            if self._resp is not None: self._resp.close()
        except Exception: pass

    def _build(self):
        p, base = self.p, trim_url(self.p.base_url)
        hdr = {"Content-Type": "application/json", "Accept": "text/event-stream"}
        if p.kind == ANTHROPIC:
            url = base + "/messages"; hdr.update({"x-api-key": self.key, "anthropic-version": "2023-06-01"})
            msgs = []
            for m in self.msgs:
                parts = []
                for ip in m.images:
                    mime, b64 = encode_image(ip); parts.append({"type": "image", "source": {"type": "base64", "media_type": mime, "data": b64}})
                if m.text: parts.append({"type": "text", "text": m.text})
                msgs.append({"role": m.role, "content": parts})
            mt = max(256, self.max_tokens); body = {"model": self.model, "messages": msgs, "stream": True}
            if self.system: body["system"] = self.system
            if self.thinking > 0 and thinking_supported(p.kind, self.model):
                bud = [0, 2048, 8192, 16000][min(self.thinking, 3)]; body["thinking"] = {"type": "enabled", "budget_tokens": bud}; mt = max(mt, bud + 4096)
            body["max_tokens"] = mt
        elif p.kind == GEMINI:
            mdl = self.model[7:] if self.model.startswith("models/") else self.model
            url = f"{base}/models/{mdl}:streamGenerateContent?alt=sse"; hdr["x-goog-api-key"] = self.key
            contents = []
            for m in self.msgs:
                parts = [{"text": m.text}] if m.text else []
                for ip in m.images:
                    mime, b64 = encode_image(ip); parts.append({"inline_data": {"mime_type": mime, "data": b64}})
                contents.append({"role": "model" if m.role == "assistant" else "user", "parts": parts})
            body = {"contents": contents}
            if self.system: body["systemInstruction"] = {"parts": [{"text": self.system}]}
            if self.thinking > 0 and thinking_supported(p.kind, self.model):
                body["generationConfig"] = {"thinkingConfig": {"thinkingBudget": [0, 1024, 8192, 24576][min(self.thinking, 3)]}}
        else:
            url = base if base.endswith("/chat/completions") else base + "/chat/completions"
            if self.key: hdr["Authorization"] = "Bearer " + self.key
            if p.org and p.kind in (OPENAI, COMPAT): hdr["OpenAI-Organization"] = p.org
            msgs = [{"role": "system", "content": self.system}] if self.system else []
            for m in self.msgs:
                if not m.images: msgs.append({"role": m.role, "content": m.text}); continue
                parts = [{"type": "text", "text": m.text}] if m.text else []
                for ip in m.images:
                    mime, b64 = encode_image(ip); parts.append({"type": "image_url", "image_url": {"url": f"data:{mime};base64,{b64}"}})
                msgs.append({"role": m.role, "content": parts})
            body = {"model": self.model, "messages": msgs, "stream": True}
            if p.kind == OPENAI and thinking_supported(p.kind, self.model): body["reasoning_effort"] = ["low", "medium", "high", "high"][min(self.thinking, 3)]
        return urllib.request.Request(url, data=json.dumps(body).encode("utf-8"), headers=hdr, method="POST")

    def _emit(self, t):
        if t: self.got = True; self.delta.emit(t)

    def _handle(self, o):
        if o.get("error"):
            if not self.err: self.err = server_message(json.dumps(o).encode()) or "Sağlayıcı bir hata bildirdi."
            return
        k = self.p.kind
        if k == ANTHROPIC:
            t = o.get("type")
            if t == "content_block_delta" and (o.get("delta") or {}).get("type") == "text_delta": self._emit(o["delta"].get("text", ""))
            elif t == "message":
                for c in o.get("content", []): self._emit(c.get("text", ""))
        elif k == GEMINI:
            pf = o.get("promptFeedback") or {}
            if pf.get("blockReason") and not self.err: self.err = f"İstek sağlayıcı tarafından engellendi ({pf['blockReason']})."
            for c in o.get("candidates", []):
                for part in (c.get("content") or {}).get("parts", []):
                    if not part.get("thought"): self._emit(part.get("text", ""))
                if c.get("finishReason") in ("SAFETY", "RECITATION", "BLOCKLIST", "PROHIBITED_CONTENT") and not self.got and not self.err:
                    self.err = f"Yanıt güvenlik filtresi nedeniyle engellendi ({c['finishReason']})."
        else:
            for c in o.get("choices", []):
                d = (c.get("delta") or {}).get("content")
                if isinstance(d, str): self._emit(d)
                elif isinstance(c.get("message"), dict): self._emit(c["message"].get("content") or "")
                elif isinstance(c.get("text"), str): self._emit(c["text"])

    def _line(self, line):
        if not line.startswith("data:"): return
        pl = line[5:].strip()
        if not pl or pl == "[DONE]": return
        try: o = json.loads(pl)
        except ValueError: return
        if isinstance(o, dict): self._handle(o)

    def run(self):
        try:
            err = validate_url(self.p.base_url)
            if err: self.failed.emit(err); return
            try: req = self._build()
            except ValueError as e: self.failed.emit(str(e)); return
            try:
                self._resp = urllib.request.urlopen(req, timeout=max(5, self.p.timeout))
            except urllib.error.HTTPError as e:
                self.failed.emit(api_error_text(e.code, e, server_message(e.read()), self.p, self.key)); return
            ctype = self._resp.headers.get("Content-Type", "")
            if "event-stream" in ctype:
                for raw in self._resp:
                    if self._abort: break
                    self._line(raw.decode("utf-8", "replace").strip())
            else:
                body = self._resp.read()
                if not self._abort:
                    txt = body.decode("utf-8", "replace")
                    if txt.lstrip().startswith("data:") or "\ndata:" in txt:
                        for ln in txt.splitlines(): self._line(ln.strip())
                    else:
                        try: d = json.loads(txt)
                        except ValueError: self.failed.emit("Sunucu beklenen biçimde yanıt vermedi (JSON/SSE değil). Base URL'yi kontrol edin."); return
                        for o in (d if isinstance(d, list) else [d]):
                            if isinstance(o, dict): self._handle(o)
            if self._abort: self.done.emit(); return
            if self.err: self.failed.emit(self.err); return
            if not self.got: self.failed.emit("Sağlayıcı boş bir yanıt döndürdü."); return
            self.done.emit()
        except Exception as e:  # ağ hataları, kesilen bağlantılar
            if self._abort: self.done.emit()
            else: self.failed.emit(api_error_text(0, e, "", self.p, self.key))
        finally:
            try:
                if self._resp is not None: self._resp.close()
            except Exception: pass


class TestWorker(QThread):
    result = Signal(str, bool, str, list)

    def __init__(self, prov, key):
        super().__init__(); self.p, self.key = prov, key

    def run(self):
        p, key, base = self.p, self.key, trim_url(self.p.base_url)
        hdr = {}
        if p.kind == GEMINI: url = base + "/models?pageSize=200"; hdr["x-goog-api-key"] = key
        elif p.kind == ANTHROPIC: url = base + "/models?limit=100"; hdr.update({"x-api-key": key, "anthropic-version": "2023-06-01"})
        else:
            if base.endswith("/chat/completions"): base = base[:-17]
            url = base + "/models"
            if key: hdr["Authorization"] = "Bearer " + key
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=hdr), timeout=max(5, p.timeout)) as r: body = r.read()
        except urllib.error.HTTPError as e:
            self.result.emit(p.id, False, api_error_text(e.code, e, server_message(e.read()), p, key), []); return
        except Exception as e:
            self.result.emit(p.id, False, api_error_text(0, e, "", p, key), []); return
        try: d = json.loads(body)
        except ValueError: self.result.emit(p.id, False, "Sunucu yanıt verdi ancak beklenen JSON biçiminde değil. Base URL yanlış olabilir.", []); return
        ms = []
        if p.kind == GEMINI:
            for m in d.get("models", []):
                if "generateContent" in m.get("supportedGenerationMethods", []): ms.append(m.get("name", "").removeprefix("models/"))
        else:
            ms = [x.get("id", "") for x in d.get("data", []) if isinstance(x, dict)]
            if not ms:
                for x in d.get("models", []): ms.append(x if isinstance(x, str) else (x.get("id") or x.get("model") or x.get("name") or ""))
        ms = [m for m in ms if m]
        msg = f"Bağlantı başarılı — {len(ms)} model listelendi." if ms else "Bağlantı başarılı; sağlayıcı model listesi döndürmedi."
        if ms and p.model and p.model not in ms: msg += f"\nUyarı: '{p.model}' model kimliği listede bulunamadı."
        self.result.emit(p.id, True, msg, ms)


JOBS = set()


def start_thread(t):
    JOBS.add(t); t.finished.connect(lambda: JOBS.discard(t)); t.start()


class Api(QObject):
    changed = Signal()
    test_done = Signal(str, bool, str, list)

    def __init__(self):
        super().__init__()
        try: self.provs = [prov_from_dict(d) for d in json.loads(Cfg.get("providers", "[]"))]
        except Exception: self.provs = []
        self.default = Cfg.get("defaultProvider", "")

    def find(self, pid): return next((p for p in self.provs if p.id == pid), None)
    def key(self, pid): return Secrets.load("prov:" + pid)
    def key_for(self, p): return LocalServer.key() if p.kind == LOCAL else self.key(p.id)
    def _persist(self): Cfg.set("providers", json.dumps([prov_to_dict(p) for p in self.provs])); Cfg.set("defaultProvider", self.default)

    def save(self, c, key, key_changed):
        c.name = c.name.strip() or PK_NAMES[c.kind]
        e = validate_url(c.base_url)
        if e: return e
        old = self.find(c.id)
        if old: c.last_ok, c.last_msg, c.last_tested = old.last_ok, old.last_msg, old.last_tested; self.provs[self.provs.index(old)] = c
        else: self.provs.append(c)
        if key_changed and not Secrets.save("prov:" + c.id, key) and key:
            BUS.error.emit("API anahtarı Windows Kimlik Bilgisi Yöneticisi'ne kaydedilemedi (keyring kurulu mu?); yalnızca bu oturumda geçerli.")
        if not self.default: self.default = c.id
        self._persist(); self.changed.emit(); return ""

    def remove(self, pid):
        self.provs = [p for p in self.provs if p.id != pid]; Secrets.save("prov:" + pid, "")
        if self.default == pid: self.default = self.provs[0].id if self.provs else ""
        self._persist(); self.changed.emit()

    def set_default(self, pid): self.default = pid; self._persist(); self.changed.emit()

    def test(self, c, key):
        e = validate_url(c.base_url)
        if not e and c.kind in KEY_REQUIRED and not key.strip(): e = "API anahtarı girilmemiş."
        if e: QTimer.singleShot(0, lambda: self.test_done.emit(c.id, False, e, [])); return
        w = TestWorker(c, key)

        def fin(pid, ok, msg, ms):
            p = self.find(pid)
            if p: p.last_ok, p.last_msg, p.last_tested = ok, msg, now_ms(); self._persist(); self.changed.emit()
            self.test_done.emit(pid, ok, msg, ms)
        w.result.connect(fin); start_thread(w)


class LocalServer(QObject):
    status_changed = Signal()
    test_done = Signal(bool, str, list)
    state = 0  # 0 test edilmedi, 1 bağlı, 2 hata
    msg = ""
    _key = None
    inst = None

    @staticmethod
    def address(): return Cfg.get("localAddr", "http://127.0.0.1")
    @staticmethod
    def port(): return int(Cfg.get("localPort", 8765))
    @staticmethod
    def model(): return Cfg.get("localModel", "")
    @staticmethod
    def models(): return Cfg.get("localModels", [])

    @classmethod
    def key(cls):
        if cls._key is None: cls._key = Secrets.load("local")
        return cls._key

    @staticmethod
    def normalize(a):
        a = a.strip()
        if "://" not in a: a = "http://" + a
        return a.rstrip("/")

    @classmethod
    def validate(cls, addr, port):
        if not addr.strip() or not re.match(r"^https?://[^/\s:]+", cls.normalize(addr)): return "Geçersiz sunucu adresi. Örnek: http://127.0.0.1"
        if not (1 <= int(port) <= 65535): return "Geçersiz bağlantı noktası (1–65535)."
        return ""

    @classmethod
    def base_url(cls):
        m = re.match(r"^(https?://[^/:]+)(?::\d+)?(.*)$", cls.normalize(cls.address()))
        return f"{m.group(1)}:{cls.port()}{m.group(2)}/v1" if m else ""

    @classmethod
    def save(cls, addr, port, key, model):
        e = cls.validate(addr, port)
        if e: return e
        Cfg.set("localAddr", cls.normalize(addr)); Cfg.set("localPort", int(port)); Cfg.set("localModel", model.strip()); cls._key = key
        if not Secrets.save("local", key) and key: BUS.error.emit("Sunucu anahtarı Windows Kimlik Bilgisi Yöneticisi'ne kaydedilemedi; yalnızca bu oturumda geçerli.")
        cls.state = 0; cls.inst.status_changed.emit(); return ""

    @classmethod
    def cfg(cls):
        return Prov("local", PK_NAMES[LOCAL], LOCAL, cls.base_url(), cls.model(), "", int(Cfg.get("localTimeout", 120)), -1, cls.state == 1)

    @classmethod
    def test(cls, addr, port, key):
        e = cls.validate(addr, port)
        if e: QTimer.singleShot(0, lambda: cls._fin(False, e, [])); return
        m = re.match(r"^(https?://[^/:]+)(?::\d+)?(.*)$", cls.normalize(addr))
        p = Prov("local", "local", LOCAL, f"{m.group(1)}:{int(port)}{m.group(2)}/v1", "", "", 8)
        w = TestWorker(p, key)
        w.result.connect(lambda pid, ok, msg, ms: cls._fin(ok, msg, ms)); start_thread(w)

    @classmethod
    def _fin(cls, ok, msg, models):
        cls.state = 1 if ok else 2; cls.msg = msg
        if ok:
            Cfg.set("localModels", models)
            if not cls.model() and models: Cfg.set("localModel", models[0])
        cls.inst.status_changed.emit(); cls.inst.test_done.emit(ok, msg, models)


# ============================================================================
# 4) Donanım izleme, bellek tahmini, model kataloğu
# ============================================================================
GIB = 1073741824.0


class SmiWorker(QThread):
    result = Signal(object)

    def run(self):
        for exe in ("nvidia-smi", r"C:\Windows\System32\nvidia-smi.exe", r"C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe"):
            try:
                flags = 0x08000000 if os.name == "nt" else 0
                r = subprocess.run([exe, "--query-gpu=name,memory.total,memory.used,memory.free,utilization.gpu", "--format=csv,noheader,nounits"],
                                   capture_output=True, text=True, timeout=5, creationflags=flags)
            except (FileNotFoundError, OSError):
                continue
            except subprocess.TimeoutExpired:
                self.result.emit(None); return
            gl = []
            for ln in r.stdout.strip().splitlines():
                f = [x.strip() for x in ln.split(",")]
                if len(f) < 4: continue
                num = lambda s: int(s) if s.isdigit() else -1
                gl.append({"name": f[0], "total": num(f[1]), "used": num(f[2]), "free": num(f[3]), "util": num(f[4]) if len(f) > 4 else -1})
            self.result.emit(gl if (r.returncode == 0 and gl) else None); return
        self.result.emit(None)


class MEMSTAT(ctypes.Structure):
    _fields_ = [("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong), ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                ("ullTotalPageFile", ctypes.c_ulonglong), ("ullAvailPageFile", ctypes.c_ulonglong), ("ullTotalVirtual", ctypes.c_ulonglong),
                ("ullAvailVirtual", ctypes.c_ulonglong), ("sullAvailExtendedVirtual", ctypes.c_ulonglong)]


class Hardware(QObject):
    updated = Signal()

    def __init__(self):
        super().__init__()
        self.cpu_name = self._cpu_name(); self.cores = os.cpu_count() or 0; self.cpu = -1.0
        self.ram_total = self.ram_used = self.ram_avail = -1
        self.gpus = []; self.gpu_ok = False; self.gpu_msg = "Unavailable"
        self.refs = 0; self._prev = None; self._smi = None; self._missing = False
        self.timer = QTimer(self); self.timer.setInterval(2000); self.timer.timeout.connect(self.refresh)
        self.refresh()

    @staticmethod
    def _cpu_name():
        if os.name == "nt":
            try:
                import winreg
                with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as k:
                    return " ".join(str(winreg.QueryValueEx(k, "ProcessorNameString")[0]).split())
            except Exception: pass
        return "Unavailable"

    def acquire(self):
        self.refs += 1
        if self.refs == 1: self.timer.start(); self.refresh()

    def release(self):
        self.refs = max(0, self.refs - 1)
        if self.refs == 0: self.timer.stop()

    def vram_total_gb(self): return sum(g["total"] for g in self.gpus if g["total"] > 0) / 1024 if self.gpu_ok else -1
    def vram_free_gb(self): return sum(g["free"] for g in self.gpus if g["free"] >= 0) / 1024 if self.gpu_ok else -1
    def ram_avail_gb(self): return self.ram_avail / GIB if self.ram_avail > 0 else -1

    def refresh(self):
        if os.name == "nt":
            try:
                ms = MEMSTAT(); ms.dwLength = ctypes.sizeof(ms)
                if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(ms)):
                    self.ram_total, self.ram_avail = ms.ullTotalPhys, ms.ullAvailPhys; self.ram_used = self.ram_total - self.ram_avail
                f = [ctypes.c_ulonglong() for _ in range(3)]
                if ctypes.windll.kernel32.GetSystemTimes(*[ctypes.byref(x) for x in f]):
                    i, k, u = (x.value for x in f)
                    if self._prev:
                        di, dt = i - self._prev[0], (k - self._prev[1]) + (u - self._prev[2])
                        if dt > 0: self.cpu = max(0.0, min(100.0, 100.0 * (1 - di / dt)))
                    self._prev = (i, k, u)
            except Exception: pass
        elif Path("/proc/meminfo").exists():
            try:
                d = {l.split(":")[0]: int(l.split()[1]) * 1024 for l in Path("/proc/meminfo").read_text().splitlines() if ":" in l}
                self.ram_total, self.ram_avail = d.get("MemTotal", -1), d.get("MemAvailable", -1); self.ram_used = self.ram_total - self.ram_avail
            except Exception: pass
        if not self._missing and (self._smi is None or not self._smi.isRunning()):
            self._smi = SmiWorker(); self._smi.result.connect(self._on_smi); self._smi.start()
        self.updated.emit()

    def _on_smi(self, gl):
        if gl is None: self.gpu_ok = False; self.gpus = []; self.gpu_msg = "Unavailable (nvidia-smi bulunamadı veya veri döndürmedi; NVIDIA GPU/sürücü gerekli)"; self._missing = True
        else: self.gpus = gl; self.gpu_ok = True; self.gpu_msg = ""
        self.updated.emit()


HW = None


@dataclass
class ModelEntry:
    id: str = ""; name: str = ""; family: str = ""; kind: int = LOCAL; prov_id: str = ""; api_id: str = ""; quant: str = ""
    params_b: float = -1; ctx: int = 0; layers: int = 0; kv_heads: int = 0; head_dim: int = 0
    vram_override: float = -1; ram_override: float = -1; builtin: bool = False; demo: bool = False; vision: int = -1

    def location(self): return "Yerel" if self.kind == LOCAL else ("API" if self.kind in (COMPAT, CUSTOM) else "Bulut")


BPW = {"F32": 32, "F16": 16, "BF16": 16, "FP16": 16, "Q8_0": 8.5, "Q8": 8.5, "FP8": 8, "INT8": 8, "Q6_K": 6.6, "Q6": 6.6, "Q5_K_M": 5.7, "Q5": 5.7,
       "Q4_K_M": 4.85, "Q4": 4.5, "Q4_0": 4.5, "INT4": 4.0, "FP4": 4.0, "Q3_K_M": 3.9, "Q3": 3.9}


def estimate_mem(e):
    """dict(known, demo, weights, kv, overhead, vram, ram)"""
    m = {"known": False, "demo": False, "weights": -1, "kv": -1, "overhead": 1.0, "vram": -1, "ram": -1}
    if e.vram_override > 0 or e.ram_override > 0:
        m.update(known=True, demo=True, vram=e.vram_override, ram=e.ram_override); return m
    b = BPW.get(e.quant.strip().upper(), -1)
    if e.params_b <= 0 or b <= 0: return m
    m["weights"] = e.params_b * 1e9 * b / 8 / GIB
    if e.ctx > 0 and e.layers > 0 and e.kv_heads > 0 and e.head_dim > 0: m["kv"] = 2.0 * e.layers * e.kv_heads * e.head_dim * e.ctx * 2.0 / GIB
    tot = m["weights"] + max(0, m["kv"]) + m["overhead"]
    m.update(known=True, vram=tot, ram=tot); return m


def fit_of(e):
    if e.kind != LOCAL: return "api" if e.kind in (COMPAT, CUSTOM) else "cloud"
    m = estimate_mem(e)
    if not m["known"]: return "unknown"
    vf, rf = HW.vram_free_gb(), HW.ram_avail_gb()
    if vf >= 0:
        if 0 < m["vram"] <= vf: return "vram"
        if rf >= 0: return "offload" if m["vram"] <= vf + rf else "insufficient"
        return "unknown"
    if rf >= 0 and m["ram"] > 0: return "ram" if m["ram"] <= rf else "insufficient"
    return "unknown"


FIT_TEXT = {"cloud": "Bulut modeli", "api": "API modeli", "vram": "VRAM'e sığar", "offload": "RAM'e boşaltma ile sığar", "ram": "Yalnızca RAM (CPU) ile sığar",
            "insufficient": "Yetersiz bellek", "unknown": "Bilinmiyor"}


def fit_color(f): return {"vram": T.ok, "offload": T.accent, "ram": T.accent, "insufficient": T.danger}.get(f, T.text2)


def builtin_models():
    def mk(i, name, fam, kind, prov, pb, q, ctx, **kw): return ModelEntry(i, name, fam, kind, prov, quant=q, params_b=pb, ctx=ctx, builtin=True, **kw)
    return [mk("b.local", "RÜZGAR Local", "RÜZGAR Local Server", LOCAL, "local", -1, "", 0),
            mk("b.deepseek", "DeepSeek", "DeepSeek API", DEEPSEEK, "", -1, "", 0, api_id="deepseek-chat"),
            mk("b.qwen", "Qwen", "Örnek profil · yerel", LOCAL, "local", 7, "Q4_K_M", 8192),
            mk("b.llama", "Llama", "Örnek profil · yerel", LOCAL, "local", 8, "Q4_K_M", 8192),
            mk("b.mistral", "Mistral", "Örnek profil · yerel", LOCAL, "local", 7, "Q4_K_M", 8192),
            mk("b.demo70", "Örnek 70B Q4", "Gösterim değerleri", LOCAL, "local", 70, "Q4", 8192, vram_override=48, ram_override=32, demo=True),
            mk("b.custom", "Custom Model", "Özel uç nokta", CUSTOM, "", -1, "", 0)]


class Catalog(QObject):
    changed = Signal()
    active_changed = Signal()

    def __init__(self):
        super().__init__()
        self.custom = [ModelEntry(**{k: v for k, v in d.items() if k in ModelEntry.__dataclass_fields__}) for d in Cfg.get("customModels", [])]
        API.changed.connect(self.changed); LocalServer.inst.status_changed.connect(self.changed)

    def all(self):
        has_ds = any(p.kind == DEEPSEEK for p in API.provs); has_cu = any(p.kind == CUSTOM for p in API.provs)
        out = [b for b in builtin_models() if not (b.id == "b.deepseek" and has_ds) and not (b.id == "b.custom" and has_cu)]
        out += [ModelEntry("cfg:" + p.id, p.model or p.name, p.name, p.kind, p.id, p.model) for p in API.provs]
        return out + self.custom

    def by_id(self, i): return next((e for e in self.all() if e.id == i), ModelEntry())
    def active(self):
        l = self.all(); aid = Cfg.get("activeModel", "b.local"); return next((e for e in l if e.id == aid), l[0])
    def active_id(self): return self.active().id
    def set_active(self, i): Cfg.set("activeModel", i); self.active_changed.emit(); self.changed.emit()
    def _persist(self): Cfg.set("customModels", [{k: getattr(e, k) for k in ModelEntry.__dataclass_fields__} for e in self.custom])

    def add_custom(self, e):
        if not e.id: e.id = "c:" + new_id()
        self.custom.append(e); self._persist(); self.changed.emit(); return e.id

    def update_custom(self, e):
        self.custom = [e if c.id == e.id else c for c in self.custom]; self._persist(); self.changed.emit()

    def remove_custom(self, i):
        self.custom = [c for c in self.custom if c.id != i]
        if Cfg.get("activeModel") == i: Cfg.set("activeModel", "b.local")
        self._persist(); self.changed.emit()

    def resolve(self, e):
        """(Prov | None)"""
        if e.prov_id == "local" or (not e.prov_id and e.kind == LOCAL): return LocalServer.cfg()
        if e.prov_id: return API.find(e.prov_id)
        return next((p for p in API.provs if p.kind == e.kind), None)

    def api_model(self, e, p): return e.api_id or p.model

    def vision_of(self, e, p):
        if e.vision >= 0: return e.vision
        if p.vision >= 0: return p.vision
        return vision_heuristic(p.kind, self.api_model(e, p))

    def status_of(self, e):
        """(metin, seviye 0 gri/1 iyi/2 kötü)"""
        p = self.resolve(e)
        if p is None: return "Yapılandırılmadı", 2
        if p.kind == LOCAL: return ("Bağlı", 1) if LocalServer.state == 1 else (("Bağlı değil", 2) if LocalServer.state == 2 else ("Test edilmedi", 0))
        if p.last_tested == 0: return "Test edilmedi", 0
        return ("Bağlantı doğrulandı", 1) if p.last_ok else ("Bağlantı hatası", 2)


API = None
CAT = None


# ============================================================================
# 5) Temel widget'lar
# ============================================================================
def col_of(role):
    return {"text": T.text, "text2": T.text2, "white": "#FFFFFF", "accent": T.accent, "danger": T.danger}.get(role, role)


class IBtn(QPushButton):
    """İkonlu / metinli düz buton. variant: ghost | primary | danger | nav | (varsayılan outline)."""

    def __init__(self, icon="", text="", variant="ghost", size=20, color="text2", parent=None, tip=""):
        super().__init__(text, parent)
        self.icon_name, self.isz, self.crole = icon, size, color
        self.setProperty("v", variant); self.setCursor(Qt.PointingHandCursor); self.setFocusPolicy(Qt.NoFocus)
        if tip: self.setToolTip(tip)
        BUS.theme_changed.connect(self.refresh_icon); self.refresh_icon()

    def refresh_icon(self):
        if self.icon_name:
            self.setIcon(QIcon(pix(self.icon_name, self.isz, col_of(self.crole)))); self.setIconSize(QSize(self.isz, self.isz))

    def set_icon_name(self, n, role=None):
        self.icon_name = n
        if role: self.crole = role
        self.refresh_icon()

    def square(self, w=36, h=None, round_=False):
        h = h or w; self.setFixedSize(w, h)
        self.setStyleSheet(f"padding:0;border-radius:{(min(w, h) // 2) if round_ else 10}px;"); return self


class Popup(QFrame):
    M = 14

    def __init__(self, parent=None):
        super().__init__(parent, Qt.Popup | Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint)
        self.setAttribute(Qt.WA_TranslucentBackground); self.setAttribute(Qt.WA_DeleteOnClose)
        self.lay = vbox(self, 0, 2); self.lay.setContentsMargins(self.M + 6, self.M + 6, self.M + 6, self.M + 6)

    def show_near(self, anchor, above=False, align_right=False):
        self.lay.activate(); self.adjustSize(); M = self.M
        ga = anchor.mapToGlobal(QPoint(0, 0))
        x = ga.x() + anchor.width() - self.width() + M if align_right else ga.x() - M
        y = ga.y() - self.height() + M - 6 if above else ga.y() + anchor.height() - M + 6
        av = anchor.screen().availableGeometry()
        x = max(av.left(), min(x, av.right() - self.width())); y = max(av.top(), min(y, av.bottom() - self.height()))
        fin = QRect(x, y, self.width(), self.height())
        if Cfg.anim():
            st = QRect(fin.x() + int(fin.width() * .03), fin.y() + (int(fin.height() * .06) if above else 0), int(fin.width() * .94), int(fin.height() * .94))
            self.setWindowOpacity(0.0); self.setGeometry(st); self.show()
            g = QParallelAnimationGroup(self)
            a1 = QPropertyAnimation(self, b"geometry"); a1.setDuration(150); a1.setStartValue(st); a1.setEndValue(fin); a1.setEasingCurve(QEasingCurve.OutCubic)
            a2 = QPropertyAnimation(self, b"windowOpacity"); a2.setDuration(150); a2.setStartValue(0.0); a2.setEndValue(1.0)
            g.addAnimation(a1); g.addAnimation(a2); g.start(); self._g = g
        else:
            self.setWindowOpacity(1.0); self.setGeometry(fin); self.show()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); r = QRectF(self.rect()).adjusted(self.M, self.M, -self.M, -self.M); p.setPen(Qt.NoPen)
        for i in range(6, 0, -1):
            p.setBrush(QColor(0, 0, 0, 22 if T.dark else 12)); p.drawRoundedRect(r.adjusted(-i * 1.6, -i * 1.6 + 3, i * 1.6, i * 1.6 + 3), 16 + i, 16 + i)
        p.setBrush(QColor(T.composer if T.dark else T.bg)); p.setPen(QPen(QColor(T.border), 1)); p.drawRoundedRect(r, 14, 14)


class MenuRow(QAbstractButton):
    hovered = Signal()

    def __init__(self, icon, title, sub="", parent=None):
        super().__init__(parent)
        self.icon_name, self.title, self.sub, self.is_check, self.is_danger = icon, title, sub, False, False
        self.setCursor(Qt.PointingHandCursor); self.setFocusPolicy(Qt.NoFocus); BUS.theme_changed.connect(self.update)

    def check(self, b): self.is_check = b; self.update(); return self
    def danger(self, b=True): self.is_danger = b; self.update(); return self
    def sizeHint(self): return QSize(240, 56 if self.sub else 38)
    def minimumSizeHint(self): return QSize(120, self.sizeHint().height())
    def enterEvent(self, e): super().enterEvent(e); self.hovered.emit(); self.update()
    def leaveEvent(self, e): super().leaveEvent(e); self.update()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); h = self.height()
        if self.underMouse():
            p.setPen(Qt.NoPen); p.setBrush(QColor(T.hover)); p.drawRoundedRect(QRectF(self.rect()).adjusted(0, 1, 0, -1), 8, 8)
        fg = QColor(T.danger if self.is_danger else T.text); x = 12
        if self.icon_name: p.drawPixmap(x, (h - 20) // 2, pix(self.icon_name, 20, fg.name())); x += 30
        right = self.width() - 12
        if self.is_check: p.drawPixmap(right - 18, (h - 18) // 2, pix("check", 18, T.accent)); right -= 26
        avail = max(20, right - x); tf = QFont(self.font()); tf.setPixelSize(14); p.setPen(fg)
        if not self.sub:
            p.setFont(tf); p.drawText(QRect(x, 0, avail, h), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(tf).elidedText(self.title, Qt.ElideRight, avail))
        else:
            tf.setWeight(QFont.Medium); p.setFont(tf); p.drawText(QRect(x, 8, avail, 20), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(tf).elidedText(self.title, Qt.ElideRight, avail))
            sf = QFont(self.font()); sf.setPixelSize(12); p.setFont(sf); p.setPen(QColor(T.text2))
            p.drawText(QRect(x, 29, avail, 18), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(sf).elidedText(self.sub, Qt.ElideRight, avail))


class TypingIndicator(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent); self.setFixedSize(46, 24)
        self.a = QVariantAnimation(self); self.a.setStartValue(0.0); self.a.setEndValue(1.0); self.a.setDuration(1100); self.a.setLoopCount(-1)
        self.a.valueChanged.connect(lambda: self.update())

    def showEvent(self, e):
        if Cfg.anim(): self.a.start()

    def hideEvent(self, e): self.a.stop()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing)
        t = self.a.currentValue() if self.a.state() == QAbstractAnimation.Running else 0.5
        for i in range(3):
            s = .5 + .5 * math.sin(((t - i * .18) % 1.0) * 2 * math.pi); c = QColor(T.text2); c.setAlphaF(.35 + .65 * s)
            p.setPen(Qt.NoPen); p.setBrush(c); p.drawEllipse(QPointF(10 + 13 * i, 12), 3.2 + 1.2 * s, 3.2 + 1.2 * s)


class AvatarWidget(QWidget):
    def __init__(self, size, parent=None):
        super().__init__(parent); self.setFixedSize(size, size); self.ini = ""

    def set_initials(self, s): self.ini = s; self.update()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); p.setPen(Qt.NoPen); p.setBrush(QColor("#8A93A3")); p.drawEllipse(self.rect())
        f = QFont(self.font()); f.setPixelSize(int(self.height() * .38)); f.setWeight(QFont.DemiBold); p.setFont(f); p.setPen(QColor("white")); p.drawText(self.rect(), Qt.AlignCenter, self.ini)


class LogoLabel(QLabel):
    def __init__(self, size, parent=None):
        super().__init__(parent); self.s = size; self.setFixedSize(size, size); self.setPixmap(logo_pix(size)); BUS.theme_changed.connect(lambda: self.setPixmap(logo_pix(self.s)))


class RoundImage(QWidget):
    clicked = Signal()

    def __init__(self, pm, size, radius, lbl, parent=None):
        super().__init__(parent); self.pm, self.r, self.lbl, self.click = pm, radius, lbl, False; self.setFixedSize(size, size)

    def set_clickable(self, c): self.click = c; self.setCursor(Qt.PointingHandCursor if c else Qt.ArrowCursor)

    def mouseReleaseEvent(self, e):
        if self.click and e.button() == Qt.LeftButton and self.rect().contains(e.position().toPoint()): self.clicked.emit()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); path = QPainterPath(); path.addRoundedRect(QRectF(self.rect()), self.r, self.r); p.setClipPath(path)
        if self.pm is None or self.pm.isNull():
            p.fillRect(self.rect(), QColor(T.hover)); p.setPen(QColor(T.text2)); f = QFont(self.font()); f.setPixelSize(11); p.setFont(f)
            p.drawText(self.rect().adjusted(4, 4, -4, -4), Qt.AlignCenter | Qt.TextWordWrap, self.lbl)
        else: p.drawPixmap(self.rect(), self.pm)
        p.setClipping(False); p.setBrush(Qt.NoBrush); p.setPen(QPen(QColor(T.border), 1)); p.drawRoundedRect(QRectF(self.rect()).adjusted(.5, .5, -.5, -.5), self.r, self.r)


def thumb_pix(path, sz):
    rd = QImageReader(str(path)); rd.setAutoTransform(True); img = rd.read()
    if img.isNull(): return QPixmap()
    img = img.scaled(sz * 2, sz * 2, Qt.KeepAspectRatioByExpanding, Qt.SmoothTransformation)
    img = img.copy((img.width() - sz * 2) // 2, (img.height() - sz * 2) // 2, sz * 2, sz * 2)
    pm = QPixmap.fromImage(img); pm.setDevicePixelRatio(2); return pm


class SmoothScroll(QScrollArea):
    bottom_state = Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent); self.setWidgetResizable(True); self.setFrameShape(QFrame.NoFrame); self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.stick = True; self._prog = False; self._last = True
        self.anim = QPropertyAnimation(self.verticalScrollBar(), b"value", self); self.anim.setEasingCurve(QEasingCurve.OutCubic); self.anim.setDuration(220)
        self.verticalScrollBar().rangeChanged.connect(self._range); self.verticalScrollBar().valueChanged.connect(self._value)

    def _range(self, *_):
        if self.stick: self._to_bottom()
        self._report()

    def _value(self, v):
        if not self._prog: self.stick = v >= self.verticalScrollBar().maximum() - 40
        self._report()

    def _to_bottom(self): self._prog = True; self.verticalScrollBar().setValue(self.verticalScrollBar().maximum()); self._prog = False

    def _report(self):
        sb = self.verticalScrollBar(); b = sb.maximum() - sb.value() < 120
        if b != self._last: self._last = b; self.bottom_state.emit(b)

    def set_stick(self, s):
        self.stick = s
        if s: self._to_bottom()

    def scroll_bottom(self, animated):
        self.stick = True; sb = self.verticalScrollBar()
        if not animated or not Cfg.anim(): self._to_bottom(); return
        self.anim.stop(); self.anim.setStartValue(sb.value()); self.anim.setEndValue(sb.maximum()); self.anim.start()

    def wheelEvent(self, e):
        if not Cfg.anim() or e.angleDelta().y() == 0 or not e.pixelDelta().isNull(): return super().wheelEvent(e)
        sb = self.verticalScrollBar(); base = self.anim.endValue() if self.anim.state() == QAbstractAnimation.Running else sb.value()
        tgt = max(sb.minimum(), min(base - e.angleDelta().y(), sb.maximum())); self.stick = False
        self.anim.stop(); self.anim.setStartValue(sb.value()); self.anim.setEndValue(tgt); self.anim.start(); e.accept()


# ============================================================================
# 6) Sözdizimi renklendirme, kod blokları, Markdown, mesajlar
# ============================================================================
def W(s): return set(s.split())


class LD:
    def __init__(self, **k):
        d = dict(kw=set(), types=set(), lit=set(), lc=[], cblock=False, triple=False, backtick=False, pre=False, ci=False, cap=False, dollar=False, charonly=False, macros=False, rules=None)
        d.update(k); self.__dict__.update(d)


_STR = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
LANGS = {}
_c_kw = "alignas alignof asm auto break case catch class const constexpr const_cast continue decltype default delete do dynamic_cast else enum explicit export extern final for friend goto if inline mutable namespace new noexcept operator override private protected public register reinterpret_cast return sizeof static static_assert static_cast struct switch template this throw try typedef typeid typename union using virtual volatile while concept requires co_await co_return co_yield"
LANGS["python"] = LD(kw=W("and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield match case"),
                     lit=W("True False None self cls"), types=W("int str float bool list dict set tuple bytes object print len range type"), lc=["#"], triple=True)
LANGS["cpp"] = LD(kw=W(_c_kw), types=W("int long short char float double bool void signed unsigned size_t string vector map unordered_map set std wchar_t int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t"),
                  lit=W("true false nullptr NULL"), lc=["//"], cblock=True, pre=True, charonly=True)
LANGS["c"] = LD(kw=W("auto break case const continue default do else enum extern for goto if inline register restrict return sizeof static struct switch typedef union volatile while"), types=LANGS["cpp"].types,
                lit=W("NULL true false"), lc=["//"], cblock=True, pre=True, charonly=True)
LANGS["java"] = LD(kw=W("abstract assert break case catch class const continue default do else enum extends final finally for goto if implements import instanceof interface native new package private protected public return static strictfp super switch synchronized this throw throws transient try var volatile while record"),
                   types=W("boolean byte char double float int long short void String Integer List Map Object System"), lit=W("true false null"), lc=["//"], cblock=True, charonly=True, cap=True)
_js_kw = "async await break case catch class const continue debugger default delete do else export extends finally for from function if import in instanceof let new of return static super switch this throw try typeof var void while with yield"
LANGS["javascript"] = LD(kw=W(_js_kw), lit=W("true false null undefined NaN"), types=W("console Promise Array Object Map Set JSON Math"), lc=["//"], cblock=True, backtick=True)
LANGS["typescript"] = LD(kw=W(_js_kw + " interface type enum implements namespace declare abstract readonly private public protected as is keyof infer satisfies"), lit=W("true false null undefined NaN"),
                         types=W("console Promise Array Object Map Set JSON Math string number boolean any unknown never object bigint symbol"), lc=["//"], cblock=True, backtick=True, cap=True)
LANGS["bash"] = LD(kw=W("if then else elif fi for while do done case esac function in select until return exit export local readonly echo cd set unset source alias param foreach"), lit=W("true false"), lc=["#"], dollar=True)
LANGS["sql"] = LD(kw=W("select from where insert into values update set delete create table alter drop index view join inner outer left right full cross on as and or not null is in like between group by order having limit offset union all distinct case when then else end primary key foreign references default unique check constraint exists asc desc begin commit rollback"),
                  types=W("int integer bigint smallint varchar char text boolean date timestamp datetime float double decimal numeric serial"), lit=W("true false"), lc=["--"], cblock=True, ci=True)
LANGS["rust"] = LD(kw=W("as async await break const continue crate dyn else enum extern fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait type unsafe use where while"),
                   types=W("i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 bool char str String Vec Option Result Box Some None Ok Err"), lit=W("true false"), lc=["//"], cblock=True, charonly=True, macros=True, cap=True)
LANGS["go"] = LD(kw=W("break case chan const continue default defer else fallthrough for func go goto if import interface map package range return select struct switch type var"),
                 types=W("int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 float32 float64 string bool byte rune error any"), lit=W("true false nil iota"), lc=["//"], cblock=True, backtick=True, charonly=True)
LANGS["json"] = LD(rules=[(_STR, "str"), (r'"(?:\\.|[^"\\])*"(?=\s*:)', "key"), (r"-?\b\d[\d.eE+-]*", "num"), (r"\b(?:true|false|null)\b", "lit")])
LANGS["yaml"] = LD(rules=[(r"^\s*-?\s*[\w.\-]+(?=\s*:)", "key"), (_STR, "str"), (r"\b\d[\d.]*\b", "num"), (r"\b(?:true|false|null|yes|no|on|off)\b", "lit"), (r"#.*$", "com")])
LANGS["html"] = LD(rules=[(r"</?[\w:-]+", "kw"), (r"/?>", "kw"), (r"[\w:@-]+(?==)", "key"), (_STR, "str"), (r"<!--.*?-->", "com")])
LANGS["css"] = LD(rules=[(r"[^{}:;]+(?=\{)", "type"), (r"[\w-]+(?=\s*:)", "key"), (r"#[0-9a-fA-F]{3,8}\b", "num"), (r"\b\d[\w.%]*", "num"), (_STR, "str"), (r"/\*.*?\*/", "com")])
ALIASES = {"c++": "cpp", "cc": "cpp", "cxx": "cpp", "hpp": "cpp", "h": "c", "js": "javascript", "jsx": "javascript", "mjs": "javascript", "node": "javascript", "ts": "typescript", "tsx": "typescript",
           "py": "python", "python3": "python", "sh": "bash", "shell": "bash", "zsh": "bash", "powershell": "bash", "ps1": "bash", "console": "bash", "yml": "yaml", "xml": "html", "svg": "html",
           "htm": "html", "rs": "rust", "golang": "go", "jsonc": "json", "scss": "css", "postgresql": "sql", "mysql": "sql", "sqlite": "sql"}
LABELS = {"cpp": "C++", "c": "C", "python": "Python", "java": "Java", "javascript": "JavaScript", "typescript": "TypeScript", "html": "HTML", "css": "CSS", "json": "JSON", "yaml": "YAML", "bash": "Bash", "sql": "SQL", "rust": "Rust", "go": "Go"}


def canon_lang(l):
    l = re.split(r"[\s{:]", (l or "").strip().lower())[0]; return ALIASES.get(l, l)


class Highlighter(QSyntaxHighlighter):
    def __init__(self, doc, lang):
        super().__init__(doc); self.d = LANGS.get(canon_lang(lang)); self.refresh(); BUS.theme_changed.connect(self._theme)

    def _theme(self): self.refresh(); self.rehighlight()

    def refresh(self):
        def mk(l, d, it=False):
            f = QTextCharFormat(); f.setForeground(QColor(d if T.dark else l)); f.setFontItalic(it); return f
        self.f = {"kw": mk("#0000FF", "#569CD6"), "str": mk("#A31515", "#CE9178"), "com": mk("#008000", "#6A9955", True), "num": mk("#098658", "#B5CEA8"), "type": mk("#267F99", "#4EC9B0"),
                  "func": mk("#795E26", "#DCDCAA"), "lit": mk("#0000FF", "#569CD6"), "pre": mk("#AF00DB", "#C586C0"), "key": mk("#0451A5", "#9CDCFE"), "var": mk("#001080", "#9CDCFE")}

    def highlightBlock(self, text):
        d = self.d
        if d is None: return
        F = self.f
        if d.rules:
            for pat, name in d.rules:
                for m in re.finditer(pat, text, re.M): self.setFormat(m.start(), m.end() - m.start(), F[name])
            return
        st = max(self.previousBlockState(), 0); i = 0; n = len(text); self.setCurrentBlockState(0)
        if st == 1:
            e = text.find("*/")
            if e < 0: self.setFormat(0, n, F["com"]); self.setCurrentBlockState(1); return
            self.setFormat(0, e + 2, F["com"]); i = e + 2
        elif st in (2, 3):
            q = '"""' if st == 2 else "'''"; e = text.find(q)
            if e < 0: self.setFormat(0, n, F["str"]); self.setCurrentBlockState(st); return
            self.setFormat(0, e + 3, F["str"]); i = e + 3
        ident = re.compile(r"[A-Za-z_]\w*"); num = re.compile(r"\d[\w.]*")
        while i < n:
            c = text[i]
            if d.pre and c == "#" and not text[:i].strip(): self.setFormat(i, n - i, F["pre"]); return
            if any(text.startswith(p, i) for p in d.lc): self.setFormat(i, n - i, F["com"]); return
            if d.cblock and text.startswith("/*", i):
                e = text.find("*/", i + 2)
                if e < 0: self.setFormat(i, n - i, F["com"]); self.setCurrentBlockState(1); return
                self.setFormat(i, e + 2 - i, F["com"]); i = e + 2; continue
            if d.triple and (text.startswith('"""', i) or text.startswith("'''", i)):
                q = text[i:i + 3]; e = text.find(q, i + 3)
                if e < 0: self.setFormat(i, n - i, F["str"]); self.setCurrentBlockState(2 if q[0] == '"' else 3); return
                self.setFormat(i, e + 3 - i, F["str"]); i = e + 3; continue
            if c in "\"'" or (d.backtick and c == "`"):
                j = i + 1
                while j < n and text[j] != c: j += 2 if text[j] == "\\" else 1
                if j >= n:
                    if c == "'": i += 1; continue
                    self.setFormat(i, n - i, F["str"]); return
                if c == "'" and d.charonly and j - i > 4: i += 1; continue
                self.setFormat(i, j + 1 - i, F["str"]); i = j + 1; continue
            if c.isdigit() and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")):
                m = num.match(text, i); self.setFormat(i, m.end() - i, F["num"]); i = m.end(); continue
            if d.dollar and c == "$":
                j = i + 1
                if j < n and text[j] == "{": e = text.find("}", j); j = n if e < 0 else e + 1
                else:
                    while j < n and (text[j].isalnum() or text[j] == "_"): j += 1
                self.setFormat(i, j - i, F["var"]); i = j; continue
            m = ident.match(text, i)
            if m:
                w = m.group(); k = w.lower() if d.ci else w; j = m.end(); f = None
                if k in d.kw: f = F["kw"]
                elif k in d.lit: f = F["lit"]
                elif k in d.types: f = F["type"]
                elif d.macros and j < n and text[j] == "!": f = F["func"]
                elif j < n and text[j] == "(": f = F["func"]
                elif d.cap and w[0].isupper(): f = F["type"]
                if f: self.setFormat(i, j - i, f)
                i = j; continue
            i += 1


class CodeEdit(QPlainTextEdit):
    def wheelEvent(self, e):
        if abs(e.angleDelta().y()) >= abs(e.angleDelta().x()): e.ignore()
        else: super().wheelEvent(e)


class CodeBlock(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent); self.setObjectName("codeBox"); v = vbox(self)
        self.head = QFrame(self); self.head.setObjectName("codeHead"); self.head.setFixedHeight(38); h = hbox(self.head, 0, 4); h.setContentsMargins(14, 0, 8, 0)
        self.lang_lbl = label("", "small", self.head)
        self.toggle = IBtn("chevup", "", "ghost", 16, "text2", self.head, "Daralt / genişlet").square(30, 28)
        self.copy_btn = IBtn("copy", " Kopyala", "ghost", 15, "text2", self.head); self.copy_btn.setFixedSize(100, 28)
        h.addWidget(self.lang_lbl); h.addStretch(1); h.addWidget(self.toggle); h.addWidget(self.copy_btn)
        self.ed = CodeEdit(self); self.ed.setObjectName("codeBody"); self.ed.setReadOnly(True); self.ed.setUndoRedoEnabled(False); self.ed.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.ed.setFrameShape(QFrame.NoFrame); self.ed.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded); self.ed.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        v.addWidget(self.head); v.addWidget(self.ed)
        self.code = ""; self.lang = None; self.hl = None; self.collapsed = False
        self.copy_btn.clicked.connect(self._copy); self.toggle.clicked.connect(self._toggle)

    def _copy(self):
        QApplication.clipboard().setText(self.code); self.copy_btn.set_icon_name("check"); self.copy_btn.setText(" Kopyalandı")
        QTimer.singleShot(1500, lambda: (self.copy_btn.set_icon_name("copy"), self.copy_btn.setText(" Kopyala")))

    def _toggle(self):
        self.collapsed = not self.collapsed; self.ed.setVisible(not self.collapsed); self.toggle.set_icon_name("chevdown" if self.collapsed else "chevup")

    def set_code(self, code, lang):
        if lang != self.lang:
            self.lang = lang; self.lang_lbl.setText(LABELS.get(canon_lang(lang), lang.strip() or "Kod"))
            if self.hl is not None: self.hl.setDocument(None); self.hl.deleteLater()
            self.ed.clear(); self.code = ""; self.hl = Highlighter(self.ed.document(), lang)
        if code != self.code:
            if self.code and code.startswith(self.code):
                c = QTextCursor(self.ed.document()); c.movePosition(QTextCursor.End); c.insertText(code[len(self.code):])
            else: self.ed.setPlainText(code)
            self.code = code
        self.ed.ensurePolished(); fm = self.ed.fontMetrics(); self.ed.setTabStopDistance(4 * fm.horizontalAdvance(" "))
        self.ed.setFixedHeight(max(1, self.ed.document().blockCount()) * fm.lineSpacing() + int(self.ed.document().documentMargin() * 2) + 16)


FENCE_RE = re.compile(r"^\s*(```|~~~)\s*(.*)$")


def split_md(src):
    out = []; text = []; code = []; inc = False; lang = ""; fence = ""
    for ln in src.split("\n"):
        m = FENCE_RE.match(ln)
        if not inc:
            if m: 
                if "".join(text).strip(): out.append((False, "", "\n".join(text)))
                text = []; inc = True; fence = m.group(1); lang = m.group(2).strip(); code = []; continue
            text.append(ln)
        else:
            if m and m.group(1) == fence and not m.group(2).strip(): out.append((True, lang, "\n".join(code))); inc = False; continue
            code.append(ln)
    if inc: out.append((True, lang, "\n".join(code)))
    elif "".join(text).strip(): out.append((False, "", "\n".join(text)))
    return out


def safe_md(text):
    """Ham HTML etiketlerini etkisizleştir (satır içi kod dışında)."""
    parts = re.split(r"(`+[^`]*`+)", text)
    return "".join(p if p.startswith("`") else re.sub(r"<(?=[A-Za-z/!?])", "&lt;", p) for p in parts)


class MarkdownView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent); self.lay = vbox(self, 0, 6); self.items = []; self.md = ""; self.stream = False
        BUS.theme_changed.connect(lambda: self.rebuild(True))

    def set_markdown(self, md, streaming=False):
        if md == self.md and streaming == self.stream: return
        self.md, self.stream = md, streaming; self.rebuild(False)

    def rebuild(self, force):
        bl = split_md(self.md)
        for idx, (is_code, lang, text) in enumerate(bl):
            last = idx == len(bl) - 1
            while idx < len(self.items) and self.items[idx][0] != is_code:
                self.items.pop(idx)[2].deleteLater()
            if idx < len(self.items):
                kind, key, w = self.items[idx]
                if is_code: w.set_code(text, lang)
                else:
                    k = text + (" ▍" if last and self.stream else "")
                    if force or key != k: w.setText(safe_md(k)); self.items[idx] = (kind, k, w)
                continue
            if is_code:
                w = CodeBlock(self); w.set_code(text, lang); self.items.insert(idx, (True, "", w))
            else:
                w = QLabel(self); w.setProperty("role", "msg"); w.setWordWrap(True); w.setTextFormat(Qt.MarkdownText); w.setOpenExternalLinks(True)
                w.setTextInteractionFlags(Qt.TextSelectableByMouse | Qt.LinksAccessibleByMouse); w.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Minimum)
                k = text + (" ▍" if last and self.stream else ""); w.setText(safe_md(k)); self.items.insert(idx, (False, k, w))
            self.lay.insertWidget(idx, w)
        while len(self.items) > len(bl): self.items.pop()[2].deleteLater()


def anim_opacity(eff, to, ms=140):
    if not Cfg.anim(): eff.setOpacity(to); return
    a = QPropertyAnimation(eff, b"opacity", eff); a.setDuration(ms); a.setStartValue(eff.opacity()); a.setEndValue(to); a.start(QAbstractAnimation.DeleteWhenStopped)


class MessageWidget(QWidget):
    regenerate = Signal()
    retry = Signal()
    open_settings = Signal()
    feedback_changed = Signal(int, int)
    delete_requested = Signal()

    def __init__(self, m, parent=None):
        super().__init__(parent); self.db_id, self.role, self.text, self.fb, self.is_last = m.id, m.role, m.content, m.feedback, False
        self.bubble_lbl = self.u_eff = self.typing = self.md = self.act = self.err_box = self.like = self.dis = self.regen = None
        (self._build_user if self.role == "user" else self._build_assistant)(m)

    def is_user(self): return self.role == "user"
    def play_appear(self):
        if Cfg.anim(): fade(self, True, 240)

    # --- asistan ---
    def show_typing(self, on):
        if self.typing: self.typing.setVisible(on)

    def set_streaming(self, md):
        self.text = md
        if self.typing: self.typing.hide()
        self.md.set_markdown(md, True)

    def finish(self, md):
        self.text = md
        if self.typing: self.typing.hide()
        self.md.set_markdown(md, False); self.act.setVisible(bool(md)); self.set_last(self.is_last); self._apply_fb()

    def set_error(self, msg, retry, settings):
        if self.typing: self.typing.hide()
        if self.err_box is None:
            self.err_box = QFrame(self); self.err_box.setObjectName("errBox"); v = vbox(self.err_box, 12, 8)
            self.err_lbl = label("", "err", self.err_box); self.err_lbl.setWordWrap(True); self.err_lbl.setTextInteractionFlags(Qt.TextSelectableByMouse); v.addWidget(self.err_lbl)
            self.err_row = QWidget(self.err_box); rh = hbox(self.err_row, 0, 8)
            self.retry_btn = IBtn("refresh", "Tekrar dene", "", 16, "text", self.err_row); self.set_btn = IBtn("settings", "API ayarlarını aç", "", 16, "text", self.err_row)
            rh.addWidget(self.retry_btn); rh.addWidget(self.set_btn); rh.addStretch(1); v.addWidget(self.err_row)
            self.retry_btn.clicked.connect(self.retry); self.set_btn.clicked.connect(self.open_settings); self.layout().insertWidget(2, self.err_box)
        self.err_lbl.setText(msg); self.retry_btn.setVisible(retry); self.set_btn.setVisible(settings); self.err_row.setVisible(retry or settings); self.err_box.show()

    def set_last(self, last):
        self.is_last = last
        if self.regen: self.regen.setVisible(last)

    def set_feedback(self, f): self.fb = f; self._apply_fb()

    def _act_btn(self, icon, tip, parent): return IBtn(icon, "", "ghost", 17, "text2", parent, tip).square(32, 30)

    def _copy(self, btn):
        QApplication.clipboard().setText(self.text); btn.set_icon_name("check"); QTimer.singleShot(1200, lambda: btn.set_icon_name("copy")); toast(self, "Panoya kopyalandı")

    def _build_assistant(self, m):
        v = vbox(self, 0, 6); self.typing = TypingIndicator(self); self.typing.hide()
        self.md = MarkdownView(self); self.md.set_markdown(m.content, False)
        self.act = QWidget(self); ah = hbox(self.act, 0, 2)
        cp = self._act_btn("copy", "Yanıtı kopyala", self.act); self.like = self._act_btn("thumbup", "İyi yanıt", self.act); self.dis = self._act_btn("thumbdown", "Kötü yanıt", self.act)
        self.regen = self._act_btn("refresh", "Yeniden oluştur", self.act); more = self._act_btn("more", "Diğer seçenekler", self.act)
        for b in (cp, self.like, self.dis, self.regen, more): ah.addWidget(b)
        ah.addStretch(1); v.addWidget(self.typing); v.addWidget(self.md); v.addWidget(self.act)
        self.act.setVisible(bool(m.content)); self.regen.setVisible(False); self._apply_fb()
        cp.clicked.connect(lambda: self._copy(cp))
        self.like.clicked.connect(lambda: self._set_fb(1)); self.dis.clicked.connect(lambda: self._set_fb(-1)); self.regen.clicked.connect(self.regenerate)
        more.clicked.connect(lambda: self._more(more))

    def _set_fb(self, f): self.fb = 0 if self.fb == f else f; self._apply_fb(); self.feedback_changed.emit(self.db_id, self.fb)

    def _more(self, btn):
        m = QMenu(self); a1 = m.addAction(ico("copy", 18, T.text), "Metin olarak kopyala"); a2 = m.addAction(ico("trash", 18, T.danger), "Bu yanıtı sil")
        r = m.exec(btn.mapToGlobal(QPoint(0, btn.height() + 4)))
        if r is a1: QApplication.clipboard().setText(self.text); toast(self, "Panoya kopyalandı")
        elif r is a2: self.delete_requested.emit()

    def _apply_fb(self):
        if not self.like: return
        self.like.crole = "accent" if self.fb == 1 else "text2"; self.dis.crole = "accent" if self.fb == -1 else "text2"; self.like.refresh_icon(); self.dis.refresh_icon()

    # --- kullanıcı ---
    def _build_user(self, m):
        h = hbox(self); h.addStretch(1); col = QWidget(self); v = vbox(col, 0, 6)
        if m.images:
            ir = QWidget(col); ih = hbox(ir, 0, 8)
            for p in m.images:
                ex = Path(p).exists(); ri = RoundImage(thumb_pix(p, 110) if ex else None, 110, 12, Path(p).name if ex else "Dosya bulunamadı", ir)
                if ex: ri.set_clickable(True); ri.clicked.connect(lambda p=p: QDesktopServices.openUrl(QUrl.fromLocalFile(p)))
                ih.addWidget(ri)
            v.addWidget(ir, 0, Qt.AlignRight)
        if m.content:
            self.bubble = QFrame(col); self.bubble.setObjectName("bubble"); bl = vbox(self.bubble); bl.setContentsMargins(16, 10, 16, 10)
            self.bubble_lbl = QLabel(m.content, self.bubble); self.bubble_lbl.setProperty("role", "msg"); self.bubble_lbl.setWordWrap(True); self.bubble_lbl.setTextFormat(Qt.PlainText)
            self.bubble_lbl.setTextInteractionFlags(Qt.TextSelectableByMouse); bl.addWidget(self.bubble_lbl); v.addWidget(self.bubble, 0, Qt.AlignRight)
            ua = QWidget(col); ah = hbox(ua, 0, 2); ah.addStretch(1); cb = self._act_btn("copy", "Kopyala", ua); ah.addWidget(cb); cb.clicked.connect(lambda: self._copy(cb))
            self.u_eff = QGraphicsOpacityEffect(ua); self.u_eff.setOpacity(0.0); ua.setGraphicsEffect(self.u_eff); v.addWidget(ua, 0, Qt.AlignRight)
        h.addWidget(col)

    def _relayout(self):
        if not self.bubble_lbl: return
        self.bubble_lbl.ensurePolished(); mw = max(160, int(self.width() * .80) - 36)
        r = QFontMetrics(self.bubble_lbl.font()).boundingRect(QRect(0, 0, mw, 100000), Qt.TextWordWrap | Qt.AlignLeft, self.text)
        self.bubble_lbl.setFixedWidth(min(mw, r.width() + 6))

    def resizeEvent(self, e): super().resizeEvent(e); self._relayout()
    def showEvent(self, e): super().showEvent(e); self._relayout()
    def enterEvent(self, e):
        super().enterEvent(e)
        if self.u_eff: anim_opacity(self.u_eff, 1.0)

    def leaveEvent(self, e):
        super().leaveEvent(e)
        if self.u_eff: anim_opacity(self.u_eff, 0.0)


# ============================================================================
# 7) Donanım paneli, model seçici, model düzenleme
# ============================================================================
def fmt_mb(mb): return "Unavailable" if mb < 0 else (f"{mb / 1024:.1f} GB" if mb >= 1024 else f"{mb} MB")
def fmt_gb(g): return f"{g:.1f} GB"
def fmt_params(b): return "Bilinmiyor" if b <= 0 else (f"{int(b)}B" if b == int(b) else f"{b:.1f}B")
def esc(s): return str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


class HardwarePanel(QWidget):
    def __init__(self, compact=False, parent=None):
        super().__init__(parent); self.compact = compact; self.lay = vbox(self, 0, 10 if compact else 18)
        self.cpu = None if compact else self._meter("CPU"); self.ram = self._meter("RAM"); self.vram = self._meter("VRAM") if compact else None
        self.gpu_host = None; self.gpus = []
        if not compact:
            self.gpu_host = QWidget(self); self.gpu_lay = vbox(self.gpu_host, 0, 18); self.lay.addWidget(self.gpu_host)
            n = label("GPU/VRAM bilgisi yalnızca NVIDIA için (nvidia-smi ile) okunur. Okunamayan değerler “Unavailable” olarak gösterilir. Değerler her 2 saniyede bir yenilenir.", "small", self); n.setWordWrap(True); self.lay.addWidget(n)
        self._acq = [False]; HW.updated.connect(self._tick); BUS.theme_changed.connect(self.refresh)
        flag = self._acq; self.destroyed.connect(lambda *_: HW.release() if flag[0] else None)

    def _meter(self, title, add=True):
        box = QWidget(self); v = vbox(box, 0, 5); head = label(title, "small" if self.compact else "h2", box); bar = QProgressBar(box); bar.setRange(0, 1000); bar.setTextVisible(False)
        det = label("", "small" if self.compact else "muted", box); det.setWordWrap(True); v.addWidget(head); v.addWidget(bar); v.addWidget(det)
        if add: self.lay.addWidget(box)
        return (box, head, bar, det)

    def showEvent(self, e):
        super().showEvent(e)
        if not self._acq[0]: self._acq[0] = True; HW.acquire()
        self.refresh()

    def hideEvent(self, e):
        super().hideEvent(e)
        if self._acq[0]: self._acq[0] = False; HW.release()

    def _tick(self):
        if self.isVisible(): self.refresh()

    def refresh(self):
        h = HW
        if self.cpu:
            self.cpu[1].setText(f"CPU — {h.cpu_name}"); self.cpu[2].setValue(0 if h.cpu < 0 else int(h.cpu * 10))
            self.cpu[3].setText(f"Kullanım: {'Unavailable' if h.cpu < 0 else f'{h.cpu:.0f} %'}   ·   Mantıksal çekirdek: {h.cores}")
        self.ram[2].setValue(int(1000 * h.ram_used / h.ram_total) if h.ram_total > 0 else 0)
        self.ram[3].setText(f"Toplam: {fmt_bytes(h.ram_total)}   ·   Kullanılan: {fmt_bytes(h.ram_used)}   ·   Kullanılabilir: {fmt_bytes(h.ram_avail)}" if h.ram_total > 0 else "Unavailable")
        if self.compact:
            tot, fr = h.vram_total_gb(), h.vram_free_gb()
            if tot > 0: self.vram[2].setValue(int(1000 * (tot - fr) / tot)); self.vram[3].setText(f"Toplam: {fmt_gb(tot)}   ·   Boş: {fmt_gb(fr)}")
            else: self.vram[2].setValue(0); self.vram[3].setText("Unavailable")
            return
        want = len(h.gpus) if h.gpu_ok else 1
        if len(self.gpus) != want:
            for g in self.gpus: g[0].deleteLater()
            self.gpus = []
            for _ in range(want): g = self._meter("GPU", False); self.gpu_lay.addWidget(g[0]); self.gpus.append(g)
        if not h.gpu_ok: self.gpus[0][1].setText("GPU"); self.gpus[0][2].setValue(0); self.gpus[0][3].setText(h.gpu_msg or "Unavailable"); return
        for i, g in enumerate(h.gpus):
            self.gpus[i][1].setText(f"GPU {i} — {g['name']}"); self.gpus[i][2].setValue(int(1000 * g["used"] / g["total"]) if g["total"] > 0 and g["used"] >= 0 else 0)
            self.gpus[i][3].setText(f"VRAM toplam: {fmt_mb(g['total'])}   ·   Kullanılan: {fmt_mb(g['used'])}   ·   Boş: {fmt_mb(g['free'])}   ·   GPU kullanımı: {'Unavailable' if g['util'] < 0 else str(g['util']) + ' %'}")


def mem_summary(e):
    if e.kind != LOCAL: return "Uygulanamaz (model sağlayıcı tarafında çalışır)."
    m = estimate_mem(e)
    if not m["known"]: return "Bilinmiyor — tahmin için parametre sayısı ve kuantizasyon gerekir."
    if m["demo"]: return f"Örnek (demo) değerler: VRAM {fmt_gb(m['vram'])}, RAM {fmt_gb(m['ram'])}. Gerçek ölçüm değildir."
    s = f"≈ {fmt_gb(m['vram'])} (tahmin) = ağırlıklar ≈ {fmt_gb(m['weights'])}"
    s += f" + KV önbelleği ≈ {fmt_gb(m['kv'])}" if m["kv"] > 0 else " + KV önbelleği (mimari bilgisi yok, dahil değil)"
    return s + f" + çalışma payı ≈ {fmt_gb(m['overhead'])} (sabit tahmin)"


def model_detail_html(e):
    p = CAT.resolve(e); st, lv = CAT.status_of(e); stc = T.ok if lv == 1 else (T.danger if lv == 2 else T.text2)
    ft = fit_of(e); m = estimate_mem(e); k, v = T.text2, T.text

    def row(a, b, col=""): return f'<tr><td style="color:{k};padding-right:12px;" valign="top">{esc(a)}</td><td style="color:{col or v};">{esc(b)}</td></tr>'
    h = f'<p style="margin:0 0 6px 0;font-size:15px;font-weight:600;">{esc(e.name)}</p><table cellspacing="0" cellpadding="3">'
    h += row("Sağlayıcı", p.name if p else PK_NAMES[e.kind]) + row("Konum", e.location()) + row("Parametre", fmt_params(e.params_b))
    h += row("Kuantizasyon", "Uygulanamaz" if e.kind != LOCAL else (e.quant or "Bilinmiyor")) + row("Bağlam uzunluğu", str(e.ctx) if e.ctx > 0 else "Bilinmiyor")
    if e.kind == LOCAL:
        h += row("Tahmini VRAM", (fmt_gb(m["vram"]) + " (örnek)" if m["demo"] else f"≈ {fmt_gb(m['vram'])} (tahmin)") if m["known"] else "Bilinmiyor")
        h += row("Tahmini RAM", (fmt_gb(m["ram"]) + " (örnek)" if m["demo"] else f"≈ {fmt_gb(m['ram'])} (tahmin, yalnızca CPU)") if m["known"] else "Bilinmiyor")
        h += row("Ayrıntı", mem_summary(e))
    else: h += row("Tahmini VRAM / RAM", "Uygulanamaz (bulut/API)")
    h += row("Uyumluluk", FIT_TEXT[ft], fit_color(ft)) + row("Bağlantı durumu", st, stc)
    am = CAT.api_model(e, p) if p else e.api_id
    h += row("İstekte gönderilen model", am or "(henüz seçilmedi)")
    vis = CAT.vision_of(e, p) if p else -1
    h += row("Görsel desteği", "Evet" if vis == 1 else ("Hayır" if vis == 0 else "Bilinmiyor (gönderilirse sunucu karar verir)")) + "</table>"
    note = ""
    if e.demo: note = "Bu giriş yalnızca gösterim amaçlıdır; değerler gerçek bir modele ait değildir."
    elif e.kind == LOCAL: note = "Bellek değerleri model dosyasından okunmaz; parametre, kuantizasyon ve bağlam bilgisine dayalı tahmindir. Ağırlık boyutu ile toplam çalışma belleği farklıdır."
    if e.builtin and e.kind == LOCAL and e.prov_id == "local" and e.params_b > 0: note += " Bu bir örnek profildir: sohbet istekleri RÜZGAR Local Server'ın seçili modeline gider."
    if note: h += f'<p style="margin-top:8px;color:{k};font-size:12px;">{esc(note)}</p>'
    return h


class ModelRow(QAbstractButton):
    hovered = Signal(object)

    def __init__(self, e, active, parent=None):
        super().__init__(parent); self.e, self.active = e, active; self.setCursor(Qt.PointingHandCursor); self.setFocusPolicy(Qt.NoFocus); self.setFixedHeight(54); BUS.theme_changed.connect(self.update)

    def enterEvent(self, ev): super().enterEvent(ev); self.hovered.emit(self.e); self.update()
    def leaveEvent(self, ev): super().leaveEvent(ev); self.update()

    def paintEvent(self, ev):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); h = self.height()
        if self.underMouse(): p.setPen(Qt.NoPen); p.setBrush(QColor(T.hover)); p.drawRoundedRect(QRectF(self.rect()).adjusted(0, 1, 0, -1), 10, 10)
        _, lv = CAT.status_of(self.e); p.setPen(Qt.NoPen); p.setBrush(QColor(T.ok if lv == 1 else (T.danger if lv == 2 else T.text2))); p.drawEllipse(QPointF(16, h / 2), 4, 4)
        right = self.width() - 12
        if self.active: p.drawPixmap(right - 18, (h - 18) // 2, pix("check", 18, T.accent)); right -= 26
        f = QFont(self.font()); f.setPixelSize(14); f.setWeight(QFont.Medium); p.setFont(f); p.setPen(QColor(T.text)); x = 32; avail = max(20, right - x)
        p.drawText(QRect(x, 7, avail, 20), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(f).elidedText(self.e.name, Qt.ElideRight, avail))
        parts = [self.e.family] + ([fmt_params(self.e.params_b)] if self.e.params_b > 0 else []) + ([self.e.quant] if self.e.quant else []) + ([f"{self.e.ctx} bağlam"] if self.e.ctx > 0 else [])
        sf = QFont(self.font()); sf.setPixelSize(12); p.setFont(sf); p.setPen(QColor(T.text2))
        p.drawText(QRect(x, 28, avail, 18), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(sf).elidedText(" · ".join(parts), Qt.ElideRight, avail))


class ModelPopup(Popup):
    add_requested = Signal()
    manage_requested = Signal()

    def __init__(self, parent):
        super().__init__(parent); top = QWidget(self); th = hbox(top, 0, 0)
        sc = QScrollArea(top); sc.setWidgetResizable(True); sc.setFixedSize(340, 400); sc.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        host = QWidget(); self.ll = vbox(host, 4, 2); sc.setWidget(host)
        div = QFrame(top); div.setFixedWidth(1); div.setStyleSheet(f"background:{T.border};")
        right = QWidget(top); right.setFixedWidth(380); rv = vbox(right, 14, 10)
        self.detail = QLabel(right); self.detail.setWordWrap(True); self.detail.setTextFormat(Qt.RichText); self.detail.setAlignment(Qt.AlignTop | Qt.AlignLeft); self.detail.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Minimum)
        rv.addWidget(self.detail); rv.addStretch(1); rv.addWidget(label("Anlık bellek", "small", right)); rv.addWidget(HardwarePanel(True, right))
        th.addWidget(sc); th.addWidget(div); th.addWidget(right)
        foot = QWidget(self); fh = hbox(foot, 6, 6)
        add = IBtn("plus", "Özel model ekle…", "", 18, "text", foot); mng = IBtn("settings", "Model yönetimi", "", 18, "text", foot)
        fh.addWidget(add); fh.addWidget(mng); fh.addStretch(1); self.lay.addWidget(top); self.lay.addWidget(foot)
        add.clicked.connect(lambda: (self.close(), self.add_requested.emit())); mng.clicked.connect(lambda: (self.close(), self.manage_requested.emit()))
        act = CAT.active_id(); allm = CAT.all()
        for loc, title in (("Yerel", "Yerel modeller"), ("Bulut", "Bulut modelleri"), ("API", "API modelleri (uyumlu / özel)")):
            first = True
            for e in allm:
                if e.location() != loc: continue
                if first: hd = label(title, "small"); hd.setContentsMargins(10, 8, 0, 2); self.ll.addWidget(hd); first = False
                r = ModelRow(e, e.id == act); self.ll.addWidget(r)
                r.hovered.connect(lambda en: self.detail.setText(model_detail_html(en))); r.clicked.connect(lambda _=False, i=e.id: (CAT.set_active(i), self.close()))
        self.ll.addStretch(1); self.detail.setText(model_detail_html(CAT.active()))


class ModelSelector(IBtn):
    add_requested = Signal()
    manage_requested = Signal()

    def __init__(self, parent=None):
        super().__init__("chevdown", "", "ghost", 16, "text2", parent); self.setLayoutDirection(Qt.RightToLeft); self.setMaximumWidth(320)
        f = QFont(self.font()); f.setWeight(QFont.DemiBold); self.setFont(f)
        CAT.active_changed.connect(self._text); CAT.changed.connect(self._text); self.clicked.connect(self._open); self._text()

    def _text(self): self.setText(CAT.active().name + "  ")

    def _open(self):
        pop = ModelPopup(self.window()); pop.add_requested.connect(self.add_requested); pop.manage_requested.connect(self.manage_requested); pop.show_near(self, False)


QUANTS = ["", "F16", "BF16", "Q8_0", "Q6_K", "Q5_K_M", "Q4_K_M", "Q4", "Q3_K_M", "INT4", "FP8"]


class ModelEditDialog(QDialog):
    def __init__(self, init, is_new, parent=None):
        super().__init__(parent); self.e = init; self.setWindowTitle("Özel model ekle" if is_new else "Modeli düzenle"); self.setModal(True); self.setMinimumWidth(560)
        v = vbox(self, 22, 12); v.addWidget(label("Özel model ekle" if is_new else "Modeli düzenle", "h2"))
        inf = label("Bellek tahmini için gerçek model bilgilerinizi girin. Bilmediğiniz alanları boş bırakın; bilinmeyen değerler “Bilinmiyor” olarak gösterilir, uydurulmaz.", "small"); inf.setWordWrap(True); v.addWidget(inf)
        fl = QFormLayout(); fl.setHorizontalSpacing(12); fl.setVerticalSpacing(10); v.addLayout(fl)
        self.name = QLineEdit(init.name); self.prov = QComboBox(); self.prov.addItem(PK_NAMES[LOCAL], "local")
        for c in API.provs: self.prov.addItem(f"{c.name} · {PK_NAMES[c.kind]}", c.id)
        ix = self.prov.findData(init.prov_id or "local"); self.prov.setCurrentIndex(max(0, ix))
        self.api = QLineEdit(init.api_id); self.api.setPlaceholderText("Boş: sağlayıcının seçili modeli kullanılır")
        self.params = QLineEdit(str(init.params_b) if init.params_b > 0 else ""); self.params.setPlaceholderText("örn. 7 (milyar)")
        self.quant = QComboBox(); self.quant.setEditable(True); self.quant.addItems(QUANTS); self.quant.setCurrentText(init.quant)
        mk = lambda x: QLineEdit(str(x) if x > 0 else "")
        self.ctx, self.layers, self.kvh, self.hd = mk(init.ctx), mk(init.layers), mk(init.kv_heads), mk(init.head_dim)
        self.vis = QComboBox(); self.vis.addItems(["Otomatik / bilinmiyor", "Evet", "Hayır"]); self.vis.setCurrentIndex(0 if init.vision < 0 else (1 if init.vision == 1 else 2))
        for lbl, w in (("Model adı", self.name), ("Sağlayıcı / uç nokta", self.prov), ("API model kimliği", self.api), ("Parametre (milyar)", self.params), ("Kuantizasyon", self.quant), ("Bağlam uzunluğu", self.ctx),
                       ("Katman sayısı (isteğe bağlı)", self.layers), ("KV head sayısı (isteğe bağlı)", self.kvh), ("Head boyutu (isteğe bağlı)", self.hd), ("Görsel desteği", self.vis)): fl.addRow(label(lbl, "muted"), w)
        self.est = label("", "muted"); self.est.setWordWrap(True); self.err = label("", "err"); self.err.setWordWrap(True); v.addWidget(self.est); v.addWidget(self.err)
        bh = QHBoxLayout(); bh.addStretch(1); cancel = QPushButton("İptal"); ok = QPushButton("Kaydet"); ok.setProperty("v", "primary"); ok.setDefault(True); bh.addWidget(cancel); bh.addWidget(ok); v.addLayout(bh)
        cancel.clicked.connect(self.reject); ok.clicked.connect(self._ok)
        for w in (self.params, self.ctx, self.layers, self.kvh, self.hd): w.textChanged.connect(self._upd)
        self.quant.currentTextChanged.connect(self._upd); self._upd()

    def _num(self, w, what, integer=False):
        t = w.text().strip().replace(",", ".")
        if not t: return -1
        try:
            d = float(t)
            if d <= 0 or (integer and d != int(d)): raise ValueError
            return int(d) if integer else d
        except ValueError: self.err.setText(f"{what} için geçerli pozitif bir sayı girin."); return None

    def _build(self):
        self.err.clear(); e = ModelEntry(**{k: getattr(self.e, k) for k in ModelEntry.__dataclass_fields__})
        e.name = self.name.text().strip(); e.prov_id = self.prov.currentData(); e.api_id = self.api.text().strip(); e.quant = self.quant.currentText().strip()
        vals = [self._num(self.params, "Parametre"), self._num(self.ctx, "Bağlam uzunluğu", True), self._num(self.layers, "Katman sayısı", True), self._num(self.kvh, "KV head sayısı", True), self._num(self.hd, "Head boyutu", True)]
        if any(x is None for x in vals): return None
        e.params_b = vals[0]; e.ctx, e.layers, e.kv_heads, e.head_dim = (max(0, int(x)) for x in vals[1:])
        e.vision = [-1, 1, 0][self.vis.currentIndex()]
        if e.prov_id == "local": e.kind = LOCAL
        else: p = API.find(e.prov_id); e.kind = p.kind if p else CUSTOM
        return e

    def _upd(self):
        e = self._build()
        if e: self.est.setText("Bellek tahmini: " + mem_summary(e))

    def _ok(self):
        e = self._build()
        if e is None: return
        if not e.name: self.err.setText("Model adı zorunludur."); return
        self.e = e; self.accept()

    def result_entry(self): return self.e


# ============================================================================
# 8) Mesaj giriş alanı (composer)
# ============================================================================
def is_image_file(p): return Path(p).suffix.lower().lstrip(".") in ("png", "jpg", "jpeg", "webp", "bmp")


class ComposerEdit(QTextEdit):
    send_requested = Signal()
    images_dropped = Signal(list)

    def __init__(self, parent=None):
        super().__init__(parent); self.setObjectName("composerEdit"); self.setAcceptRichText(False); self.setFrameShape(QFrame.NoFrame); self.document().setDocumentMargin(0)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff); self.setPlaceholderText("Mesajınızı yazın…")

    def keyPressEvent(self, e):
        if e.key() in (Qt.Key_Return, Qt.Key_Enter):
            ctrl_mode = not Cfg.get("enterSends", True); ctrl = bool(e.modifiers() & Qt.ControlModifier); shift = bool(e.modifiers() & Qt.ShiftModifier)
            if (ctrl if ctrl_mode else (not shift and not ctrl)): self.send_requested.emit(); return
            if ctrl: self.insertPlainText("\n"); return
        super().keyPressEvent(e)

    def canInsertFromMimeData(self, s): return s.hasImage() or s.hasUrls() or s.hasText()

    def insertFromMimeData(self, s):
        if s.hasImage():
            img = s.imageData()
            if isinstance(img, QImage) and not img.isNull():
                p = ATT_DIR / f"paste_{new_id()}.png"
                if img.save(str(p), "PNG"): self.images_dropped.emit([str(p)]); return
        if s.hasUrls():
            imgs = [u.toLocalFile() for u in s.urls() if u.isLocalFile() and is_image_file(u.toLocalFile())]
            if imgs: self.images_dropped.emit(imgs); return
        if s.hasText(): self.insertPlainText(s.text())


class AttachThumb(QWidget):
    remove_requested = Signal(str)

    def __init__(self, path, parent=None):
        super().__init__(parent); self.path = path; self.setFixedSize(76, 72); self.setCursor(Qt.PointingHandCursor); self.pm = thumb_pix(path, 64); self.setToolTip(Path(path).name)

    def mouseReleaseEvent(self, e):
        if e.button() != Qt.LeftButton: return
        if QLineF(e.position(), QPointF(66, 10)).length() <= 12: self.remove_requested.emit(self.path)
        else: QDesktopServices.openUrl(QUrl.fromLocalFile(self.path))

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); ir = QRect(2, 8, 64, 64); pp = QPainterPath(); pp.addRoundedRect(QRectF(ir), 12, 12); p.save(); p.setClipPath(pp)
        if self.pm.isNull():
            p.fillRect(ir, QColor(T.hover)); p.setPen(QColor(T.text2)); p.drawText(ir, Qt.AlignCenter, Path(self.path).suffix.lstrip(".").upper())
        else: p.drawPixmap(ir, self.pm)
        p.restore(); p.setBrush(Qt.NoBrush); p.setPen(QPen(QColor(T.border), 1)); p.drawRoundedRect(QRectF(ir).adjusted(.5, .5, -.5, -.5), 12, 12)
        p.setPen(Qt.NoPen); p.setBrush(QColor(T.text)); p.drawEllipse(QPointF(66, 10), 9, 9); p.drawPixmap(QPointF(60, 4), pix("x", 12, T.bg))


class SendButton(QAbstractButton):
    IDLE, READY, BUSY = 0, 1, 2

    def __init__(self, parent=None):
        super().__init__(parent); self.setFixedSize(36, 36); self.setFocusPolicy(Qt.NoFocus); self.mode = 0; BUS.theme_changed.connect(self.update)

    def set_mode(self, m): self.mode = m; self.setCursor(Qt.ArrowCursor if m == 0 else Qt.PointingHandCursor); self.setToolTip("Durdur" if m == 2 else "Gönder"); self.update()
    def enterEvent(self, e): super().enterEvent(e); self.update()
    def leaveEvent(self, e): super().leaveEvent(e); self.update()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); fg = "#FFFFFF"
        if self.mode == 2: bg = QColor(T.text) if not self.underMouse() else blend(T.text, T.text2, .25); fg = T.bg
        elif self.mode == 1: bg = QColor(T.accent_hi if self.underMouse() else T.accent)
        else: bg = QColor("#4A4A4A" if T.dark else "#D5D5D5"); fg = "#8A8A8A" if T.dark else "#FFFFFF"
        p.setPen(Qt.NoPen); p.setBrush(bg); p.drawEllipse(self.rect().adjusted(1, 1, -1, -1)); p.drawPixmap((self.width() - 20) // 2, (self.height() - 20) // 2, pix("stop" if self.mode == 2 else "arrowup", 20, fg))


THINK_NAMES = ["Hızlı", "Dengeli", "Yüksek", "Maksimum"]
THINK_DESC = ["Ek akıl yürütme istenmez", "Orta düzey akıl yürütme", "Derin akıl yürütme", "En yüksek akıl yürütme bütçesi"]


class MessageComposer(QWidget):
    M = 14
    send_requested = Signal(str, list)
    stop_requested = Signal()
    height_changed = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); self.think = max(0, min(3, int(Cfg.get("think", 0)))); self.gen = False; self.think_ok = False; self.images = []
        root = vbox(self); root.setContentsMargins(self.M + 14, self.M + 12, self.M + 14, self.M + 8); root.setSpacing(4)
        self.strip = QWidget(self); self.strip_lay = hbox(self.strip, 0, 8); self.strip_lay.setContentsMargins(0, 0, 0, 4); self.strip_lay.addStretch(1); self.strip.hide()
        self.edit = ComposerEdit(self); self.edit.setFixedHeight(28); row = QHBoxLayout(); row.setSpacing(6); row.setContentsMargins(0, 0, 0, 0)
        self.attach = IBtn("plus", "", "ghost", 22, "text2", self, "Görsel ekle").square(36, 36, True)
        self.think_btn = IBtn("brain", "", "ghost", 18, "text2", self); self.think_btn.setFixedSize(124, 34)
        self.mic = IBtn("mic", "", "ghost", 20, "text2", self, "Sesle yazma (bu sürümde yok)").square(36, 36, True); self.send = SendButton(self)
        row.addWidget(self.attach); row.addStretch(1); row.addWidget(self.think_btn); row.addWidget(self.mic); row.addWidget(self.send)
        root.addWidget(self.strip); root.addWidget(self.edit); root.addLayout(row)
        self.hanim = QVariantAnimation(self); self.hanim.setDuration(110); self.hanim.setEasingCurve(QEasingCurve.OutCubic); self.hanim.valueChanged.connect(lambda v: self.edit.setFixedHeight(int(v)))
        self.edit.textChanged.connect(lambda: (self._adjust(), self._upd())); self.edit.send_requested.connect(lambda: None if self.gen else self._try_send()); self.edit.images_dropped.connect(self.add_images)
        self.send.clicked.connect(lambda: self.stop_requested.emit() if self.gen else self._try_send()); self.attach.clicked.connect(self._pick)
        self.mic.clicked.connect(lambda: toast(self, "Ses tanıma bu sürümde kullanılamıyor. Düğme yalnızca arayüz öğesidir.", True)); self.think_btn.clicked.connect(self._open_think)
        BUS.theme_changed.connect(self.update); self._refresh_think(); self._upd()

    def text(self): return self.edit.toPlainText().strip()
    def focus_input(self): self.edit.setFocus()
    def set_thinking_support(self, ok): self.think_ok = ok; self._refresh_think()
    def set_generating(self, g): self.gen = g; self._upd()
    def clear_input(self): self.edit.clear(); self.images = []; self._rebuild_strip(); self._upd(); self._adjust()

    def _pick(self):
        files, _ = QFileDialog.getOpenFileNames(self.window(), "Görsel seç", Cfg.get("lastImgDir", str(Path.home())), "Görseller (*.png *.jpg *.jpeg *.webp *.bmp)")
        if files: Cfg.set("lastImgDir", str(Path(files[0]).parent)); self.add_images(files)

    def add_images(self, paths):
        for src in paths:
            if len(self.images) >= 8: toast(self, "En fazla 8 görsel ekleyebilirsiniz.", True); break
            p = Path(src)
            if not p.exists() or not os.access(p, os.R_OK): toast(self, f"Görsel eklenemedi: dosya bulunamadı veya okunamıyor ({p.name}).", True); continue
            if not is_image_file(src): toast(self, f"Desteklenmeyen biçim: {p.name} (PNG, JPG, JPEG, WEBP, BMP).", True); continue
            if p.stat().st_size > 20 * 1024 * 1024: toast(self, f"Görsel çok büyük (en fazla 20 MB): {p.name}", True); continue
            dst = src
            if ATT_DIR.resolve() not in p.resolve().parents:
                dst = str(ATT_DIR / f"{new_id()}{p.suffix.lower()}")
                try: dst_p = Path(dst); dst_p.write_bytes(p.read_bytes())
                except OSError as e: toast(self, f"Görsel yüklenemedi ({p.name}): {e}", True); continue
            if dst not in self.images: self.images.append(dst)
        self._rebuild_strip(); self._upd()

    def _try_send(self):
        t = self.text()
        if t or self.images: self.send_requested.emit(t, list(self.images))

    def _upd(self): self.send.set_mode(2 if self.gen else (1 if (self.text() or self.images) else 0))

    def _adjust(self):
        d = self.edit.document(); d.setTextWidth(max(50, self.edit.viewport().width()))
        h = max(28, min(220, int(d.documentLayout().documentSize().height()) + 4)); self.edit.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn if h >= 220 else Qt.ScrollBarAlwaysOff)
        if h == self.edit.height() and self.hanim.state() != QAbstractAnimation.Running: return
        if Cfg.anim() and self.isVisible(): self.hanim.stop(); self.hanim.setStartValue(self.edit.height()); self.hanim.setEndValue(h); self.hanim.start()
        else: self.edit.setFixedHeight(h)

    def _rebuild_strip(self):
        while self.strip_lay.count() > 1:
            it = self.strip_lay.takeAt(0)
            if it.widget(): it.widget().deleteLater()
        for p in self.images:
            t = AttachThumb(p, self.strip); t.remove_requested.connect(self._remove); self.strip_lay.insertWidget(self.strip_lay.count() - 1, t)
        self.strip.setVisible(bool(self.images))

    def _remove(self, path):
        if path in self.images: self.images.remove(path)
        if ATT_DIR.resolve() in Path(path).resolve().parents:
            try: Path(path).unlink()
            except OSError: pass
        self._rebuild_strip(); self._upd()

    def _refresh_think(self):
        self.think_btn.setText(THINK_NAMES[self.think]); self.think_btn.setToolTip("Düşünme modu" if self.think_ok else "Düşünme modu — seçili model/sağlayıcı bu ayarı desteklemiyor; istekte kullanılmaz")

    def _open_think(self):
        pop = Popup(self.window()); hd = label("Düşünme modu", "small"); hd.setContentsMargins(12, 6, 0, 4); pop.lay.addWidget(hd)
        for i in range(4):
            r = MenuRow("brain", THINK_NAMES[i], THINK_DESC[i]).check(i == self.think); r.setMinimumWidth(300); r.clicked.connect(lambda _=False, i=i, pop=pop: self._set_think(i, pop)); pop.lay.addWidget(r)
        if not self.think_ok:
            n = label("Seçili model/sağlayıcı bu ayarı desteklemiyor; seçim istekte kullanılmaz.", "small"); n.setWordWrap(True); n.setContentsMargins(12, 6, 12, 6); n.setMaximumWidth(300); pop.lay.addWidget(n)
        pop.show_near(self.think_btn, True, True)

    def _set_think(self, i, pop): self.think = i; Cfg.set("think", i); self._refresh_think(); pop.close()

    def resizeEvent(self, e): super().resizeEvent(e); self._adjust(); self.height_changed.emit()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); r = QRectF(self.rect()).adjusted(self.M, self.M, -self.M, -self.M); p.setPen(Qt.NoPen)
        for i in range(6, 0, -1): p.setBrush(QColor(0, 0, 0, 20 if T.dark else 9)); p.drawRoundedRect(r.adjusted(-i * 1.4, -i * 1.4 + 3, i * 1.4, i * 1.4 + 3), 26 + i, 26 + i)
        p.setBrush(QColor(T.composer)); p.setPen(QPen(QColor(T.border), 1)); p.drawRoundedRect(r, 26, 26)


# ============================================================================
# 9) Kenar çubuğu ve sohbet başlığı
# ============================================================================
RK, RID, RICON, REXTRA, RACT = 0x101, 0x102, 0x103, 0x104, 0x105
K_NAV, K_HEADER, K_PROJECT, K_CONV, K_EMPTY, K_SPACER = range(6)


def project_list(): return list(Cfg.get("projects", []))


class SideDelegate(QStyledItemDelegate):
    def __init__(self, lst): super().__init__(lst); self.lst = lst

    def sizeHint(self, o, i):
        return QSize(o.rect.width(), {K_NAV: 40, K_HEADER: 36, K_PROJECT: 38, K_CONV: 40, K_EMPTY: 34}.get(i.data(RK), 12))

    def paint(self, p, o, i):
        kind = i.data(RK)
        if kind == K_SPACER: return
        p.save(); p.setRenderHint(QPainter.Antialiasing); hov = self.lst.hover_row == i.row(); act = bool(i.data(RACT)); r = o.rect.adjusted(6, 1, -6, -1); text = i.data(Qt.DisplayRole) or ""
        f = QFont(o.font); f.setPixelSize(14)
        if kind == K_HEADER:
            f.setPixelSize(13); f.setWeight(QFont.DemiBold); p.setFont(f); p.setPen(QColor(T.text2))
            p.drawText(QRect(r.left() + 10, r.top() + 6, r.width() - 50, r.height() - 6), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(f).elidedText(text, Qt.ElideRight, r.width() - 50))
            if i.data(REXTRA): p.drawPixmap(r.right() - 28, r.top() + 12, pix("plus", 16, T.text if hov else T.text2))
            p.restore(); return
        if kind == K_EMPTY:
            f.setPixelSize(13); p.setFont(f); c = QColor(T.text2); c.setAlphaF(.8); p.setPen(c); p.drawText(r.adjusted(10, 0, 0, 0), Qt.AlignVCenter | Qt.AlignLeft, text); p.restore(); return
        if hov or act: p.setPen(Qt.NoPen); p.setBrush(QColor(T.sel if act else T.hover)); p.drawRoundedRect(r, 9, 9)
        x = r.left() + 12; icon = i.data(RICON)
        if icon: p.drawPixmap(x, r.top() + (r.height() - 20) // 2, pix(icon, 20, T.text2 if kind == K_PROJECT else T.text)); x += 32
        right = r.right() - 8
        if kind in (K_CONV, K_PROJECT):
            if hov or act: p.drawPixmap(right - 20, r.top() + (r.height() - 20) // 2, pix("more", 20, T.text2)); right -= 28
            elif kind == K_CONV and i.data(REXTRA): p.drawPixmap(right - 14, r.top() + (r.height() - 14) // 2, pix("pin", 14, T.text2)); right -= 22
        p.setFont(f); p.setPen(QColor(T.text)); w = max(10, right - x)
        p.drawText(QRect(x, r.top(), w, r.height()), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(f).elidedText(text, Qt.ElideRight, w)); p.restore()


class SideList(QListWidget):
    activated_item = Signal(object)
    conv_menu = Signal(str, QPoint)
    project_menu = Signal(str, QPoint)
    add_project = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); self.hover_row = -1
        self.setSelectionMode(QAbstractItemView.NoSelection); self.setMouseTracking(True); self.setFrameShape(QFrame.NoFrame); self.setFocusPolicy(Qt.NoFocus)
        self.setVerticalScrollMode(QAbstractItemView.ScrollPerPixel); self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff); self.setItemDelegate(SideDelegate(self)); self.verticalScrollBar().setSingleStep(20)

    def mouseMoveEvent(self, e):
        r = self.row(self.itemAt(e.position().toPoint()))
        if r != self.hover_row: self.hover_row = r; self.viewport().update()
        super().mouseMoveEvent(e)

    def leaveEvent(self, e): self.hover_row = -1; self.viewport().update(); super().leaveEvent(e)

    def mouseReleaseEvent(self, e):
        it = self.itemAt(e.position().toPoint())
        if not it or e.button() != Qt.LeftButton: return
        kind = it.data(RK); on_right = e.position().x() > self.visualItemRect(it).right() - 42
        if kind == K_CONV and on_right: self.conv_menu.emit(it.data(RID), e.globalPosition().toPoint()); return
        if kind == K_PROJECT and on_right: self.project_menu.emit(it.data(RID), e.globalPosition().toPoint()); return
        if kind == K_HEADER:
            if it.data(REXTRA) and on_right: self.add_project.emit()
            return
        if kind in (K_EMPTY, K_SPACER): return
        self.activated_item.emit(it)

    def contextMenuEvent(self, e):
        it = self.itemAt(e.pos())
        if not it: return
        if it.data(RK) == K_CONV: self.conv_menu.emit(it.data(RID), e.globalPos())
        elif it.data(RK) == K_PROJECT: self.project_menu.emit(it.data(RID), e.globalPos())


class ProfileButton(QAbstractButton):
    def __init__(self, parent=None):
        super().__init__(parent); self.setCursor(Qt.PointingHandCursor); self.setFixedHeight(56); self.setFocusPolicy(Qt.NoFocus); self.ini = self.name = self.typ = ""; BUS.theme_changed.connect(self.update)

    def set_user(self, ini, name, typ): self.ini, self.name, self.typ = ini, name, typ; self.update()
    def enterEvent(self, e): super().enterEvent(e); self.update()
    def leaveEvent(self, e): super().leaveEvent(e); self.update()

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); h = self.height()
        if self.underMouse(): p.setPen(Qt.NoPen); p.setBrush(QColor(T.hover)); p.drawRoundedRect(QRectF(self.rect()).adjusted(6, 4, -6, -4), 10, 10)
        p.setPen(Qt.NoPen); p.setBrush(QColor("#8A93A3")); p.drawEllipse(QPointF(30, h / 2), 16, 16)
        f = QFont(self.font()); f.setPixelSize(13); f.setWeight(QFont.DemiBold); p.setFont(f); p.setPen(QColor("white")); p.drawText(QRectF(14, h / 2 - 16, 32, 32), Qt.AlignCenter, self.ini)
        nf = QFont(self.font()); nf.setPixelSize(14); nf.setWeight(QFont.DemiBold); p.setFont(nf); p.setPen(QColor(T.text)); aw = self.width() - 94
        p.drawText(QRect(54, 9, aw, 20), Qt.AlignVCenter | Qt.AlignLeft, QFontMetrics(nf).elidedText(self.name, Qt.ElideRight, aw))
        tf = QFont(self.font()); tf.setPixelSize(12); p.setFont(tf); p.setPen(QColor(T.text2)); p.drawText(QRect(54, 29, aw, 18), Qt.AlignVCenter | Qt.AlignLeft, self.typ)
        p.drawPixmap(self.width() - 34, (h - 16) // 2, pix("chevup", 16, T.text2))


class Sidebar(QWidget):
    new_chat = Signal()
    open_conv = Signal(str)
    conv_deleted = Signal(str)
    section_requested = Signal(str)
    account_action = Signal(str)
    collapse_requested = Signal()
    open_changed = Signal(bool)

    def _get_w(self): return self._w

    def _set_w(self, w):
        self._w = int(w); self.setMinimumWidth(self._w); self.setMaximumWidth(self._w); self.inner.setVisible(self._w > 0); self._layout_inner()

    panel_width = Property(int, _get_w, _set_w)

    def __init__(self, parent=None):
        super().__init__(parent); self.full = 340; self.open = bool(Cfg.get("sidebarOpen", True)); self._w = self.full if self.open else 0
        self.active_conv = self.active_section = self.project = ""
        self.inner = QWidget(self); v = vbox(self.inner)
        top = QWidget(self.inner); top.setFixedHeight(58); th = hbox(top, 0, 4); th.setContentsMargins(18, 0, 12, 0)
        self.search_btn = IBtn("search", "", "ghost", 20, "text2", top, "Sohbetlerde ara (Ctrl+K)").square(36); col = IBtn("sidebar", "", "ghost", 20, "text2", top, "Kenar çubuğunu kapat (Ctrl+B)").square(36)
        th.addWidget(LogoLabel(26, top)); th.addSpacing(6); th.addWidget(label("RÜZGAR AI", "brand", top)); th.addStretch(1); th.addWidget(self.search_btn); th.addWidget(col)
        self.search = QLineEdit(self.inner); self.search.setPlaceholderText("Sohbetlerde ara…"); self.search.setClearButtonEnabled(True)
        self.search_wrap = QWidget(self.inner); sl = hbox(self.search_wrap, 0); sl.setContentsMargins(14, 0, 14, 8); sl.addWidget(self.search); self.search_wrap.hide()
        self.list = SideList(self.inner); line = QFrame(self.inner); line.setFixedHeight(1); line.setObjectName("sbLine"); self.profile = ProfileButton(self.inner)
        v.addWidget(top); v.addWidget(self.search_wrap); v.addWidget(self.list, 1); v.addWidget(line); v.addWidget(self.profile)
        self.anim = QPropertyAnimation(self, b"panel_width", self); self.anim.setDuration(230); self.anim.setEasingCurve(QEasingCurve.InOutCubic)
        self.deb = QTimer(self); self.deb.setSingleShot(True); self.deb.setInterval(180); self.deb.timeout.connect(self.refresh)
        col.clicked.connect(self.collapse_requested); self.search_btn.clicked.connect(self.focus_search); self.search.textChanged.connect(lambda: self.deb.start())
        self.search.editingFinished.connect(self._search_done)
        self.list.activated_item.connect(self._activated); self.list.conv_menu.connect(self._conv_menu); self.list.project_menu.connect(self._project_menu)
        self.list.add_project.connect(self._add_project_dialog); self.profile.clicked.connect(self._account_menu)
        BUS.convs_changed.connect(self.refresh); BUS.user_changed.connect(lambda: (self.update_user(), self.refresh())); BUS.theme_changed.connect(self.update)
        self._set_w(self._w); self.update_user()

    def _layout_inner(self): self.inner.setGeometry(0, 0, self.full, self.height())
    def resizeEvent(self, e): super().resizeEvent(e); self._layout_inner()

    def paintEvent(self, e):
        p = QPainter(self); p.fillRect(self.rect(), QColor(T.sidebar)); p.setPen(QColor(T.border))
        if self.width() > 1: p.drawLine(self.width() - 1, 0, self.width() - 1, self.height())

    def set_full_width(self, w):
        self.full = w
        if self.open and self.anim.state() != QAbstractAnimation.Running: self._set_w(w)
        self._layout_inner()

    def set_open(self, open_, animate):
        self.open = open_; Cfg.set("sidebarOpen", open_); target = self.full if open_ else 0
        if animate and Cfg.anim(): self.anim.stop(); self.anim.setStartValue(self._w); self.anim.setEndValue(target); self.anim.start()
        else: self._set_w(target)
        self.open_changed.emit(open_)

    def set_active_conv(self, cid):
        self.active_conv = cid
        if cid: self.active_section = ""
        self.refresh()

    def set_active_section(self, s):
        self.active_section = s
        if s: self.active_conv = ""
        self.refresh()

    def focus_search(self): self.set_open(True, True); self.search_wrap.show(); self.search.show(); self.search.setFocus(); self.search.selectAll()

    def _search_done(self):
        if not self.search.text().strip() and self.search_wrap.isVisible(): self.search_wrap.hide(); self.refresh()

    def update_user(self):
        if Users.logged_in(): u = Users.cur; self.profile.set_user(u.initials, u.full_name, "Yerel hesap")

    def refresh(self):
        if not Users.logged_in(): return
        term = self.search.text().strip() if self.search_wrap.isVisible() else ""
        sv = self.list.verticalScrollBar().value(); self.list.setUpdatesEnabled(False); self.list.clear()

        def add(kind, text="", cid="", icon="", extra=False, active=False):
            it = QListWidgetItem(text); it.setData(RK, kind); it.setData(RID, cid); it.setData(RICON, icon); it.setData(REXTRA, extra); it.setData(RACT, active); it.setFlags(Qt.ItemIsEnabled); self.list.addItem(it)
        if not term:
            add(K_NAV, "Yeni sohbet", "new", "newchat")
            for key, name, icon in (("images", "Görseller", "image"), ("tasks", "Görevler", "clock"), ("library", "Kitaplık", "library"), ("discover", "Keşfet", "grid")): add(K_NAV, name, key, icon, False, self.active_section == key)
            add(K_SPACER); add(K_HEADER, "Projeler", extra=True); pr = project_list()
            if not pr: add(K_EMPTY, "Proje yok")
            for n in pr: add(K_PROJECT, n, n, "folder", False, n == self.project)
            add(K_SPACER)
        convs = Convs.list(term, "" if term else self.project)
        if term: add(K_HEADER, f"Arama sonuçları ({len(convs)})")
        elif self.project: add(K_HEADER, "Proje: " + self.project)
        t0 = int(QDateTime(QDate.currentDate(), QTime(0, 0)).toMSecsSinceEpoch()); day = 86400000; names = ["Sabitlenenler", "Bugün", "Dün", "Önceki 7 gün", "Daha eski"]; groups = [[] for _ in range(5)]
        for c in convs: groups[0 if c.pinned else (1 if c.updated >= t0 else (2 if c.updated >= t0 - day else (3 if c.updated >= t0 - 7 * day else 4)))].append(c)
        for g, lst in enumerate(groups):
            if not lst: continue
            if not term: add(K_HEADER, names[g])
            for c in lst: add(K_CONV, c.title, c.id, "", c.pinned, c.id == self.active_conv)
        if not convs: add(K_EMPTY, "Sonuç bulunamadı" if term else "Henüz sohbet yok")
        self.list.setUpdatesEnabled(True); self.list.verticalScrollBar().setValue(sv); self.list.viewport().update()

    def _activated(self, it):
        kind, cid = it.data(RK), it.data(RID)
        if kind == K_NAV: self.new_chat.emit() if cid == "new" else self.section_requested.emit(cid)
        elif kind == K_PROJECT: self.project = "" if self.project == cid else cid; self.refresh()
        elif kind == K_CONV: self.open_conv.emit(cid)

    def _add_project(self, n):
        l = project_list()
        if n not in l: l.append(n); Cfg.set("projects", l)
        self.refresh()

    def _add_project_dialog(self):
        n = ask_text(self.window(), "Yeni proje", "Proje adı:")
        if n: self._add_project(n)

    def _project_menu(self, name, pos):
        m = QMenu(self); ren = m.addAction(ico("newchat", 18, T.text), "Projeyi yeniden adlandır"); dele = m.addAction(ico("trash", 18, T.danger), "Projeyi sil (sohbetler korunur)"); r = m.exec(pos)
        if r is ren:
            n = ask_text(self.window(), "Projeyi yeniden adlandır", "Yeni ad:", name)
            if n and n != name:
                l = project_list()
                if name in l and n not in l: l[l.index(name)] = n
                Cfg.set("projects", l); Convs.rename_project(name, n); self.project = n if self.project == name else self.project; self.refresh()
        elif r is dele and confirm(self.window(), "Projeyi sil", f"“{name}” projesi silinsin mi? İçindeki sohbetler silinmez."):
            l = project_list(); l = [x for x in l if x != name]; Cfg.set("projects", l); Convs.rename_project(name, ""); self.project = "" if self.project == name else self.project; self.refresh()

    def _conv_menu(self, cid, pos):
        c = Convs.get(cid)
        if not c.id: return
        m = QMenu(self); a_open = m.addAction(ico("chat", 18, T.text), "Aç"); a_ren = m.addAction(ico("newchat", 18, T.text), "Yeniden adlandır")
        a_pin = m.addAction(ico("pin", 18, T.text), "Sabitlemeyi kaldır" if c.pinned else "Sabitle"); pm = m.addMenu(ico("folder", 18, T.text), "Projeye taşı")
        for n in project_list(): a = pm.addAction(n); a.setCheckable(True); a.setChecked(n == c.project); a.setData(("proj", n))
        if c.project: pm.addAction("Projeden çıkar").setData(("proj", ""))
        a_new = pm.addAction("Yeni proje…"); m.addSeparator(); a_del = m.addAction(ico("trash", 18, T.danger), "Sil"); r = m.exec(pos)
        if r is None: return
        if r is a_open: self.open_conv.emit(cid)
        elif r is a_ren:
            t = ask_text(self.window(), "Sohbeti yeniden adlandır", "Yeni ad:", c.title)
            if t: Convs.rename(cid, t)
        elif r is a_pin: Convs.set_pinned(cid, not c.pinned)
        elif r is a_new:
            n = ask_text(self.window(), "Yeni proje", "Proje adı:")
            if n: self._add_project(n); Convs.set_project(cid, n)
        elif r is a_del:
            if confirm(self.window(), "Sohbeti sil", f"“{c.title}” sohbeti silinsin mi? Bu işlem geri alınamaz."): Convs.remove(cid); self.conv_deleted.emit(cid)
        elif isinstance(r.data(), tuple) and r.data()[0] == "proj": Convs.set_project(cid, r.data()[1])

    def _account_menu(self):
        pop = Popup(self.window()); hd = label(Users.cur.email, "small"); hd.setContentsMargins(12, 6, 0, 4); pop.lay.addWidget(hd)
        for icon, text, key in (("user", "Hesabım", "account"), ("settings", "Ayarlar", "settings"), ("key", "API ve Bağlantılar", "api"), ("server", "Model Yönetimi", "models"), ("cpu", "Donanım Bilgisi", "hardware"), ("sun", "Görünüm", "appearance")):
            r = MenuRow(icon, text); r.setMinimumWidth(self.full - 60); r.clicked.connect(lambda _=False, k=key, p=pop: (p.close(), self.account_action.emit(k))); pop.lay.addWidget(r)
        sep = QFrame(); sep.setFixedHeight(1); sep.setStyleSheet(f"background:{T.border};"); pop.lay.addWidget(sep)
        out = MenuRow("logout", "Çıkış Yap"); out.setMinimumWidth(self.full - 60); out.clicked.connect(lambda: (pop.close(), self.account_action.emit("signout"))); pop.lay.addWidget(out)
        pop.show_near(self.profile, True)


class ChatHeader(QWidget):
    toggle_sidebar = Signal()
    new_chat = Signal()
    rename_requested = Signal()
    pin_toggle = Signal()
    delete_requested = Signal()
    share_copy = Signal()
    share_file = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); self.setFixedHeight(56); self.has = False; self.pinned = False; h = hbox(self, 0, 4); h.setContentsMargins(12, 0, 12, 0)
        self.toggle = IBtn("sidebar", "", "ghost", 20, "text2", self, "Kenar çubuğunu aç (Ctrl+B)").square(36); self.newb = IBtn("newchat", "", "ghost", 20, "text2", self, "Yeni sohbet (Ctrl+N)").square(36)
        self.title = IBtn("chevdown", "Yeni sohbet", "ghost", 16, "text2", self); self.title.setLayoutDirection(Qt.RightToLeft); self.title.setMaximumWidth(400)
        self.model = ModelSelector(self); share = IBtn("share", "", "ghost", 20, "text2", self, "Paylaş / dışa aktar").square(36); more = IBtn("more", "", "ghost", 20, "text2", self, "Diğer işlemler").square(36)
        for w in (self.toggle, self.newb, self.title, self.model): h.addWidget(w)
        h.addStretch(1); h.addWidget(share); h.addWidget(more); self.set_sidebar_open(True)
        self.toggle.clicked.connect(self.toggle_sidebar); self.newb.clicked.connect(self.new_chat); self.title.clicked.connect(lambda: self._conv_menu(self.title)); more.clicked.connect(lambda: self._conv_menu(more, True))
        share.clicked.connect(lambda: self._share(share))

    def set_title(self, t, has): self.has = has; self.title.setText((t or "Yeni sohbet") + "  "); self.title.set_icon_name("chevdown") if has else self.title.setIcon(QIcon())
    def set_pinned(self, p): self.pinned = p
    def set_sidebar_open(self, o): self.toggle.setVisible(not o); self.newb.setVisible(not o)

    def _share(self, anchor):
        m = QMenu(self); a1 = m.addAction(ico("copy", 18, T.text), "Sohbeti panoya kopyala (Markdown)"); a2 = m.addAction(ico("share", 18, T.text), "Markdown dosyası olarak kaydet…")
        a1.setEnabled(self.has); a2.setEnabled(self.has); r = m.exec(anchor.mapToGlobal(QPoint(0, anchor.height() + 4)))
        if r is a1: self.share_copy.emit()
        elif r is a2: self.share_file.emit()

    def _conv_menu(self, anchor, with_new=False):
        if not self.has and not with_new: return
        m = QMenu(self); a_new = None
        if with_new: a_new = m.addAction(ico("newchat", 18, T.text), "Yeni sohbet"); m.addSeparator()
        a_ren = m.addAction(ico("newchat", 18, T.text), "Yeniden adlandır"); a_pin = m.addAction(ico("pin", 18, T.text), "Sabitlemeyi kaldır" if self.pinned else "Sabitle"); m.addSeparator()
        a_del = m.addAction(ico("trash", 18, T.danger), "Sohbeti sil")
        for a in (a_ren, a_pin, a_del): a.setEnabled(self.has)
        r = m.exec(anchor.mapToGlobal(QPoint(0, anchor.height() + 4)))
        if r is None: return
        if r is a_new: self.new_chat.emit()
        elif r is a_ren: self.rename_requested.emit()
        elif r is a_pin: self.pin_toggle.emit()
        elif r is a_del: self.delete_requested.emit()


# ============================================================================
# 10) Sohbet alanı (mesaj akışı, üretim, dışa aktarma)
# ============================================================================
class ChatWidget(QWidget):
    conversation_activated = Signal(str)
    open_settings_requested = Signal(int)
    toggle_sidebar_requested = Signal()
    conv_deleted = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent); v = vbox(self); self.header = ChatHeader(self); self.stack = QStackedWidget(self)
        self.msgs = []; self.cur = None; self.job = None; self.conv_id = ""; self.buf = ""; self.stream_conv = self.stream_model = ""; self.down_shown = False
        # karşılama
        self.welcome = QWidget(); wv = vbox(self.welcome, 24, 12); wv.addStretch(5); wv.addWidget(LogoLabel(60, self.welcome), 0, Qt.AlignHCenter)
        t = label("Nasıl yardımcı olabilirim?", "h1", self.welcome); t.setStyleSheet("font-size:30px;font-weight:600;"); t.setAlignment(Qt.AlignCenter); wv.addWidget(t)
        self.hint = label("", "muted", self.welcome); self.hint.setAlignment(Qt.AlignCenter); self.hint.setWordWrap(True); self.hint.setMaximumWidth(560); wv.addWidget(self.hint, 0, Qt.AlignHCenter)
        self.cta = IBtn("settings", "API ve Bağlantıları aç", "", 18, "text", self.welcome); self.cta.setFixedSize(230, 38); wv.addWidget(self.cta, 0, Qt.AlignHCenter); wv.addStretch(6)
        # mesajlar
        self.scroll = SmoothScroll(self); host = QWidget(); hh = hbox(host); hh.addStretch(1); self.col = QWidget(host); self.col.setMaximumWidth(800); hh.addWidget(self.col, 100); hh.addStretch(1)
        self.col_lay = vbox(self.col); self.col_lay.setContentsMargins(20, 12, 20, 28); self.col_lay.setSpacing(26); self.col_lay.addStretch(1); self.scroll.setWidget(host)
        self.stack.addWidget(self.welcome); self.stack.addWidget(self.scroll)
        cw = QWidget(self); ch = hbox(cw); ch.setContentsMargins(12, 0, 12, 0); ch.addStretch(1); self.comp = MessageComposer(cw); self.comp.setMaximumWidth(796); ch.addWidget(self.comp, 100); ch.addStretch(1)
        foot = label("RÜZGAR AI hata yapabilir. Önemli bilgileri kontrol edin.", "small", self); foot.setAlignment(Qt.AlignCenter); foot.setContentsMargins(0, 0, 0, 8)
        v.addWidget(self.header); v.addWidget(self.stack, 1); v.addWidget(cw); v.addWidget(foot)
        self.down = IBtn("arrowdown", "", "", 18, "text", self, "En alta in").square(36, 36, True); self.down.hide()
        self.flush = QTimer(self); self.flush.setSingleShot(True); self.flush.setInterval(40); self.flush.timeout.connect(lambda: self.cur.set_streaming(self.buf) if self.cur else None)
        self.stack.installEventFilter(self)
        self.comp.send_requested.connect(self.on_send); self.comp.stop_requested.connect(self.stop_generation); self.comp.height_changed.connect(self._overlay)
        self.scroll.bottom_state.connect(lambda at: self._set_down(not at and bool(self.msgs))); self.down.clicked.connect(lambda: self.scroll.scroll_bottom(True)); self.cta.clicked.connect(lambda: self.open_settings_requested.emit(4))
        CAT.active_changed.connect(self._model_state); CAT.changed.connect(self._model_state); BUS.convs_changed.connect(self._update_header)
        h = self.header; h.toggle_sidebar.connect(self.toggle_sidebar_requested); h.new_chat.connect(lambda: (self.new_conversation(), self.conversation_activated.emit("")))
        h.rename_requested.connect(self._rename); h.pin_toggle.connect(lambda: Convs.set_pinned(self.conv_id, not Convs.get(self.conv_id).pinned) if self.conv_id else None)
        h.delete_requested.connect(self._delete); h.share_copy.connect(lambda: (QApplication.clipboard().setText(self.export_md()), toast(self, "Sohbet Markdown olarak panoya kopyalandı."))); h.share_file.connect(self._save_md)
        self.new_conversation(); self._model_state()

    def eventFilter(self, o, e):
        if o is self.stack and e.type() in (QEvent.Resize, QEvent.Move): self._overlay()
        return super().eventFilter(o, e)

    def resizeEvent(self, e): super().resizeEvent(e); self._overlay()
    def _overlay(self): g = self.stack.geometry(); self.down.move(g.center().x() - 18, g.bottom() - 46); self.down.raise_()
    def _set_down(self, on):
        if on == self.down_shown: return
        self.down_shown = on; self._overlay(); fade(self.down, on, 150)

    def _update_header(self):
        if not self.conv_id: self.header.set_title("Yeni sohbet", False); self.header.set_pinned(False)
        else: c = Convs.get(self.conv_id); self.header.set_title(c.title, True); self.header.set_pinned(c.pinned)

    def _model_state(self):
        e = CAT.active(); p = CAT.resolve(e); self.comp.set_thinking_support(bool(p) and thinking_supported(p.kind, CAT.api_model(e, p))); st, _ = CAT.status_of(e)
        need_key = bool(p) and p.kind in KEY_REQUIRED and not API.key_for(p)
        if p is None: self.hint.setText(f"Seçili model ({e.name}) için bir sağlayıcı yapılandırılmamış. Başlamak için API ve Bağlantılar bölümünden bir sağlayıcı ekleyin.")
        elif need_key: self.hint.setText(f"{p.name} için API anahtarı girilmemiş. API ve Bağlantılar bölümünden ekleyin.")
        else: self.hint.setText(f"Etkin model: {e.name} · {st}")
        self.cta.setVisible(p is None or need_key)

    def new_conversation(self):
        self.stop_generation(); self._clear(); self.conv_id = ""; self._set_down(False); self.stack.setCurrentWidget(self.welcome); fade(self.welcome, True, 420)
        self._update_header(); self._model_state(); self.comp.focus_input()

    def open_conversation(self, cid):
        if not cid or not Convs.get(cid).id: self.new_conversation(); return
        if cid == self.conv_id and self.msgs: return
        self.stop_generation(); self._clear(); self.conv_id = cid; ms = Convs.messages(cid)
        for m in ms: self._add(m, False)
        self._update_last(); self._update_header(); Cfg.set("lastConv", cid); self.stack.setCurrentWidget(self.scroll if ms else self.welcome)
        self.scroll.set_stick(True); QTimer.singleShot(0, lambda: self.scroll.scroll_bottom(False)); self.comp.focus_input()

    def _clear(self):
        for w in self.msgs: self.col_lay.removeWidget(w); w.hide(); w.deleteLater()
        self.msgs = []; self.cur = None

    def _remove(self, w):
        if w in self.msgs: self.msgs.remove(w)
        if self.cur is w: self.cur = None
        self.col_lay.removeWidget(w); w.hide(); w.deleteLater()

    def _update_last(self):
        for i, w in enumerate(self.msgs): w.set_last(i == len(self.msgs) - 1 and not w.is_user())

    def _add(self, m, animate):
        w = MessageWidget(m, self.col); self.col_lay.insertWidget(self.col_lay.count() - 1, w); self.msgs.append(w)
        w.regenerate.connect(self.regenerate_last); w.retry.connect(self.regenerate_last); w.open_settings.connect(lambda: self.open_settings_requested.emit(4))
        w.feedback_changed.connect(lambda i, f: Convs.set_feedback(i, f) if i > 0 else None); w.delete_requested.connect(lambda w=w: self._delete_msg(w))
        self.stack.setCurrentWidget(self.scroll)
        if animate: w.play_appear()
        return w

    def _delete_msg(self, w):
        if self.job and w is self.cur: return
        if w.db_id > 0: Convs.delete_message(w.db_id)
        self._remove(w); self._update_last()
        if not self.msgs: self.stack.setCurrentWidget(self.welcome); fade(self.welcome, True, 300)

    def _rename(self):
        if not self.conv_id: return
        t = ask_text(self.window(), "Sohbeti yeniden adlandır", "Yeni ad:", Convs.get(self.conv_id).title)
        if t: Convs.rename(self.conv_id, t)

    def _delete(self):
        if self.conv_id and confirm(self.window(), "Sohbeti sil", f"“{Convs.get(self.conv_id).title}” sohbeti silinsin mi? Bu işlem geri alınamaz."):
            cid = self.conv_id; self.new_conversation(); Convs.remove(cid); self.conv_deleted.emit(cid); self.conversation_activated.emit("")

    def system_prompt(self):
        parts = []; lang = Cfg.get("lang", "auto")
        if lang == "tr": parts.append("Her zaman Türkçe yanıt ver.")
        elif lang == "en": parts.append("Always respond in English.")
        if Users.cur.instructions.strip(): parts.append("Kullanıcının özel talimatları:\n" + Users.cur.instructions.strip())
        return "\n\n".join(parts)

    def on_send(self, text, images):
        if self.job: return
        e = CAT.active(); p = CAT.resolve(e)
        if images and p and CAT.vision_of(e, p) == 0:
            toast(self, f"“{e.name}” görsel girdisini desteklemiyor. Görselleri kaldırın veya görsel destekli bir model seçin (sağlayıcı ayarlarında “Görsel desteği” de değiştirilebilir). İstek gönderilmedi.", True); return
        if not self.conv_id:
            self.conv_id = Convs.create("Yeni sohbet", e.id)
            if not self.conv_id: return
            self.conversation_activated.emit(self.conv_id)
        um = Msg(0, self.conv_id, "user", text, now_ms(), list(images)); um.id = Convs.add_message(um)
        if um.id == 0: return
        self.comp.clear_input(); self._add(um, True); self.scroll.scroll_bottom(True)
        if Convs.get(self.conv_id).title == "Yeni sohbet":
            t = " ".join(text.split()) or "Görsel sohbeti"; Convs.rename(self.conv_id, t[:48].strip() + ("…" if len(t) > 48 else ""))
        self.generate()

    def regenerate_last(self):
        if self.job or not self.msgs: return
        last = self.msgs[-1]
        if not last.is_user():
            if last.db_id > 0: Convs.delete_message(last.db_id)
            self._remove(last)
        self.generate()

    def generate(self):
        e = CAT.active(); p = CAT.resolve(e); mw = self._add(Msg(0, self.conv_id, "assistant", "", 0, [], e.name), True); self.cur = mw; mw.show_typing(True); self._update_last(); self.scroll.scroll_bottom(True)

        def fail(msg): mw.set_error(msg, True, True); self.cur = None
        if p is None: return fail("Seçili model için bir sağlayıcı yapılandırılmamış. Ayarlar → API ve Bağlantılar bölümünden sağlayıcıyı ekleyin ve kaydedin.")
        key = API.key_for(p)
        if p.kind in KEY_REQUIRED and not key.strip(): return fail(f"{p.name} için API anahtarı girilmemiş (geçersiz anahtar).")
        verr = validate_url(p.base_url)
        if verr: return fail("Geçersiz sunucu adresi: " + p.base_url if p.kind == LOCAL else verr)
        api_model = CAT.api_model(e, p)
        if not api_model.strip(): return fail("RÜZGAR Local Server için model seçilmedi. API ve Bağlantılar → RÜZGAR Local Server bölümünde bağlantıyı test edip bir model seçin." if p.kind == LOCAL else "Model kimliği boş (eksik model).")
        vis = CAT.vision_of(e, p); hist = Convs.messages(self.conv_id); mh = int(Cfg.get("maxHist", 0))
        if mh > 0 and len(hist) > mh: hist = hist[-mh:]
        msgs = []
        for m in hist:
            cm = ChatMsg(m.role, m.content)
            if m.images:
                if vis == 0: cm.text += "\n[Kullanıcı bir görsel ekledi; seçili model görsel desteklemediği için gönderilmedi.]"
                else:
                    for ip in m.images:
                        if Path(ip).exists(): cm.images.append(ip)
                        else: cm.text += "\n[Bir görsel dosyası artık mevcut değil, gönderilemedi.]"
            msgs.append(cm)
        self.buf = ""; self.stream_conv, self.stream_model = self.conv_id, e.name
        job = ApiWorker(p, key, api_model, msgs, self.comp.think, int(Cfg.get("maxTokens", 4096)), self.system_prompt()); self.job = job; self.comp.set_generating(True)
        job.delta.connect(lambda t, j=job: self._on_delta(j, t)); job.done.connect(lambda j=job: self._on_end(j, "", False)); job.failed.connect(lambda m, j=job: self._on_end(j, m, True)); start_thread(job)

    def _on_delta(self, job, t):
        if self.job is not job: return
        self.buf += t
        if Cfg.get("streamUi", True):
            if self.cur: self.cur.show_typing(False)
            if not self.flush.isActive(): self.flush.start()

    def stop_generation(self):
        if self.job: j = self.job; j.abort(); self._on_end(j, "", False)

    def _on_end(self, job, err, failed):
        if self.job is not job: return
        self.flush.stop(); self.job = None; self.comp.set_generating(False); mw = self.cur; self.cur = None
        if mw is None: return
        if self.buf.strip():
            mw.db_id = Convs.add_message(Msg(0, self.stream_conv, "assistant", self.buf, 0, [], self.stream_model)); mw.finish(self.buf)
            if failed: mw.set_error(err, True, True)
        elif failed: mw.finish(""); mw.set_error(err, True, True)
        else: self._remove(mw)
        self._update_last()
        if not self.msgs: self.stack.setCurrentWidget(self.welcome)

    def export_md(self):
        if not self.conv_id: return ""
        s = f"# {Convs.get(self.conv_id).title}\n\n"
        for m in Convs.messages(self.conv_id):
            s += ("**Siz:**\n\n" if m.role == "user" else f"**Asistan{' (' + m.model + ')' if m.model else ''}:**\n\n") + m.content + "\n\n---\n\n"
        return s

    def _save_md(self):
        if not self.conv_id: return
        name = re.sub(r'[\\/:*?"<>|]', "_", Convs.get(self.conv_id).title)
        path, _ = QFileDialog.getSaveFileName(self.window(), "Sohbeti kaydet", str(Path.home() / f"{name}.md"), "Markdown (*.md)")
        if not path: return
        try: Path(path).write_text(self.export_md(), "utf-8"); toast(self, "Sohbet kaydedildi.")
        except OSError as e: toast(self, f"Dosya yazılamadı: {e}", True)


# ============================================================================
# 11) Hesap ekranları (giriş / kayıt)
# ============================================================================
def pw_field(ph):
    le = QLineEdit(); le.setEchoMode(QLineEdit.Password); le.setPlaceholderText(ph)
    a = le.addAction(QIcon(pix("eye", 18, T.text2)), QLineEdit.TrailingPosition); a.setCheckable(True); a.setToolTip("Göster / gizle")

    def tog(on): le.setEchoMode(QLineEdit.Normal if on else QLineEdit.Password); a.setIcon(QIcon(pix("eyeoff" if on else "eye", 18, T.text2)))
    a.toggled.connect(tog); return le


def text_field(ph, text=""): le = QLineEdit(text); le.setPlaceholderText(ph); return le
def pbtn(text, v=None):
    b = QPushButton(text); b.setCursor(Qt.PointingHandCursor)
    if v: b.setProperty("v", v)
    return b


def link_btn(text, w):
    b = IBtn("", text, "ghost", 16, "accent", None); b.setFixedSize(w, 36); b.setStyleSheet(f"color:{T.accent};"); return b


class LoginWidget(QWidget):
    logged_in = Signal()
    switch_register = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); v = vbox(self, 0, 12); self.idf = text_field("E-posta veya kullanıcı adı"); self.pw = pw_field("Parola")
        for w in (self.idf, self.pw): w.setMinimumHeight(42)
        self.keep = QCheckBox("Bu bilgisayarda oturumu açık tut"); self.keep.setChecked(bool(Cfg.get("keepSignedIn", True)))
        self.err = label("", "err"); self.err.setWordWrap(True); self.err.hide(); self.btn = pbtn("Giriş yap", "primary"); self.btn.setMinimumHeight(42)
        sw = link_btn("Hesabınız yok mu? Kayıt olun", 250)
        for w in (self.idf, self.pw, self.keep, self.err, self.btn): v.addWidget(w)
        v.addWidget(sw, 0, Qt.AlignHCenter); self.btn.clicked.connect(self.submit); self.pw.returnPressed.connect(self.submit); self.idf.returnPressed.connect(self.pw.setFocus); sw.clicked.connect(self.switch_register)

    def reset(self): self.pw.clear(); self.err.hide(); self.idf.setFocus()

    def submit(self):
        self.err.hide(); self.btn.setEnabled(False); self.btn.setText("Giriş yapılıyor…")

        def go():
            Cfg.set("keepSignedIn", self.keep.isChecked()); e = Users.login(self.idf.text(), self.pw.text()); self.btn.setEnabled(True); self.btn.setText("Giriş yap")
            if e: self.err.setText(e); self.err.show(); self.pw.selectAll(); self.pw.setFocus(); return
            self.pw.clear(); self.logged_in.emit()
        QTimer.singleShot(30, go)


class RegisterWidget(QWidget):
    registered = Signal()
    switch_login = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); v = vbox(self, 0, 10); nr = QWidget(); nh = hbox(nr, 0, 10); self.first = text_field("Ad"); self.last = text_field("Soyad"); nh.addWidget(self.first); nh.addWidget(self.last)
        self.user = text_field("Kullanıcı adı"); self.email = text_field("E-posta"); self.pw = pw_field("Parola (en az 8 karakter)"); self.pw2 = pw_field("Parolayı doğrula")
        self.fields = [self.first, self.last, self.user, self.email, self.pw, self.pw2]
        for w in self.fields: w.setMinimumHeight(42)
        self.err = label("", "err"); self.err.setWordWrap(True); self.err.hide(); self.btn = pbtn("Hesap oluştur", "primary"); self.btn.setMinimumHeight(42); sw = link_btn("Zaten hesabınız var mı? Giriş yapın", 280)
        v.addWidget(nr)
        for w in (self.user, self.email, self.pw, self.pw2, self.err, self.btn): v.addWidget(w)
        v.addWidget(sw, 0, Qt.AlignHCenter); self.btn.clicked.connect(self.submit); self.pw2.returnPressed.connect(self.submit); sw.clicked.connect(self.switch_login)

    def reset(self):
        for w in self.fields: w.clear()
        self.err.hide(); self.first.setFocus()

    def submit(self):
        self.err.hide(); self.btn.setEnabled(False); self.btn.setText("Hesap oluşturuluyor…")

        def go():
            e = Users.register(self.first.text(), self.last.text(), self.user.text(), self.email.text(), self.pw.text(), self.pw2.text()); self.btn.setEnabled(True); self.btn.setText("Hesap oluştur")
            if e: self.err.setText(e); self.err.show(); return
            self.pw.clear(); self.pw2.clear(); self.registered.emit()
        QTimer.singleShot(30, go)


class AuthScreen(QWidget):
    authenticated = Signal()

    def __init__(self, parent=None):
        super().__init__(parent); root = vbox(self); sa = QScrollArea(self); sa.setWidgetResizable(True); sa.setFrameShape(QFrame.NoFrame)
        host = QWidget(); hv = vbox(host); hv.setContentsMargins(20, 30, 20, 30); card = QFrame(host); card.setObjectName("authCard"); card.setFixedWidth(460); cv = vbox(card, 36, 14)
        cv.addWidget(LogoLabel(52, card), 0, Qt.AlignHCenter); self.title = label("", "h1", card); self.title.setAlignment(Qt.AlignCenter); self.sub = label("", "muted", card); self.sub.setAlignment(Qt.AlignCenter); self.sub.setWordWrap(True)
        cv.addWidget(self.title); cv.addWidget(self.sub); cv.addSpacing(6); self.stack = QStackedWidget(card); self.login = LoginWidget(); self.reg = RegisterWidget(); self.stack.addWidget(self.login); self.stack.addWidget(self.reg); cv.addWidget(self.stack)
        note = label("Bu hesap yalnızca bu bilgisayarda saklanır ve cihazlar arasında eşitlenmez. Parolalar tuzlanmış PBKDF2-SHA256 ile özetlenir; düz metin olarak saklanmaz.", "small", card); note.setWordWrap(True); note.setAlignment(Qt.AlignCenter)
        cv.addSpacing(4); cv.addWidget(note); hv.addStretch(1); hv.addWidget(card, 0, Qt.AlignHCenter); hv.addStretch(1); sa.setWidget(host); root.addWidget(sa)
        self.login.logged_in.connect(self.authenticated); self.reg.registered.connect(self.authenticated)
        self.login.switch_register.connect(lambda: self.mode(True)); self.reg.switch_login.connect(lambda: self.mode(False)); BUS.theme_changed.connect(self.update)

    def start(self): self.mode(not Users.has_accounts(), False)

    def mode(self, reg, animate=True):
        if reg: self.title.setText("Hesap oluşturun"); self.sub.setText("RÜZGAR AI'yı kullanmaya başlamak için yerel bir hesap oluşturun."); self.reg.reset(); self.stack.setCurrentWidget(self.reg)
        else: self.title.setText("Tekrar hoş geldiniz"); self.sub.setText("RÜZGAR AI yerel hesabınıza giriş yapın."); self.login.reset(); self.stack.setCurrentWidget(self.login)
        if animate: fade(self.stack.currentWidget(), True, 220)

    def paintEvent(self, e): QPainter(self).fillRect(self.rect(), QColor(T.sidebar))


# ============================================================================
# 12) Ayarlar penceresi (9 kategori)
# ============================================================================
def hline(): f = QFrame(); f.setObjectName("hline"); f.setFixedHeight(1); return f


def setting_row(title, desc, ctl):
    w = QWidget(); h = hbox(w, 0, 18); h.setContentsMargins(0, 12, 0, 12); l = QWidget(w); lv = vbox(l, 0, 2); lv.addWidget(QLabel(title, l))
    if desc: d = label(desc, "small", l); d.setWordWrap(True); lv.addWidget(d)
    h.addWidget(l, 1); h.addWidget(ctl, 0, Qt.AlignRight | Qt.AlignVCenter); return w


def num_spin(lo, hi, v, w=110):
    s = QSpinBox(); s.setRange(lo, hi); s.setValue(int(v)); s.setButtonSymbols(QAbstractSpinBox.NoButtons); s.setFixedWidth(w); return s


def set_status(l, text, lv):
    l.setText(text); l.setWordWrap(True); l.setTextInteractionFlags(Qt.TextSelectableByMouse); l.setStyleSheet(f"color:{T.ok if lv == 1 else (T.danger if lv == 2 else T.text2)};")


class ThemeCard(QAbstractButton):
    def __init__(self, mode, lbl, parent=None):
        super().__init__(parent); self.mode, self.lbl = mode, lbl; self.setCheckable(True); self.setFixedSize(168, 128); self.setCursor(Qt.PointingHandCursor); self.setFocusPolicy(Qt.NoFocus); BUS.theme_changed.connect(self.update)

    def paintEvent(self, e):
        p = QPainter(self); p.setRenderHint(QPainter.Antialiasing); pr = QRectF(3, 3, self.width() - 6, self.height() - 40)

        def prev(t, r):
            path = QPainterPath(); path.addRoundedRect(r, 10, 10); p.save(); p.setClipPath(path); p.fillRect(r, QColor(t.bg)); p.fillRect(QRectF(r.left(), r.top(), r.width() * .27, r.height()), QColor(t.sidebar))
            p.setPen(Qt.NoPen); p.setBrush(QColor(t.hover)); p.drawRoundedRect(QRectF(r.left() + r.width() * .33, r.top() + 14, r.width() * .4, 8), 4, 4); p.drawRoundedRect(QRectF(r.left() + r.width() * .33, r.top() + 30, r.width() * .55, 8), 4, 4)
            p.setBrush(QColor(t.composer)); p.setPen(QPen(QColor(t.border), 1)); p.drawRoundedRect(QRectF(r.left() + r.width() * .33, r.bottom() - 26, r.width() * .6, 16), 8, 8)
            p.setPen(Qt.NoPen); p.setBrush(QColor(t.accent)); p.drawEllipse(QPointF(r.right() - 16, r.bottom() - 18), 5, 5); p.restore()
        if self.mode == 0: prev(LIGHT, pr)
        elif self.mode == 1: prev(DARK, pr)
        else:
            p.save(); p.setClipRect(QRectF(pr.left(), pr.top(), pr.width() / 2, pr.height())); prev(LIGHT, pr); p.restore()
            p.save(); p.setClipRect(QRectF(pr.center().x(), pr.top(), pr.width() / 2, pr.height())); prev(DARK, pr); p.restore()
        p.setBrush(Qt.NoBrush); p.setPen(QPen(QColor(T.accent if self.isChecked() else T.border), 2.5 if self.isChecked() else 1)); p.drawRoundedRect(pr, 10, 10)
        f = QFont(self.font()); f.setWeight(QFont.DemiBold if self.isChecked() else QFont.Normal); p.setFont(f); p.setPen(QColor(T.text)); p.drawText(QRectF(0, self.height() - 30, self.width(), 26), Qt.AlignCenter, self.lbl)


class SettingsDialog(QDialog):
    CATS = [("settings", "Genel"), ("sun", "Görünüm"), ("user", "Kişiselleştirme"), ("server", "Modeller"), ("key", "API ve Bağlantılar"), ("cpu", "Donanım"), ("chat", "Sohbet"), ("user", "Hesap"), ("info", "RÜZGAR AI Hakkında")]

    def __init__(self, parent, page):
        super().__init__(parent); self.setWindowTitle("Ayarlar"); self.resize(1040, 720); self.setMinimumSize(840, 580); self.setModal(True)
        self.sign_out = False; self.cleared = False; self._shown = False; self.loading = False; self.cur_id = ""; self.stored_key = ""; self.test_id = None; self.last_kind = OPENAI
        root = hbox(self); nav = QWidget(self); nav.setFixedWidth(256); nv = vbox(nav, 16, 4); tl = label("Ayarlar", "h2", nav); tl.setContentsMargins(10, 6, 0, 12); nv.addWidget(tl)
        self.stack = QStackedWidget(self); self.grp = QButtonGroup(self); self.grp.setExclusive(True)
        for i, (icon, name) in enumerate(self.CATS):
            b = IBtn(icon, "  " + name, "nav", 20, "text", nav); b.setCheckable(True); b.setFixedHeight(40); nv.addWidget(b); self.grp.addButton(b, i)
        nv.addStretch(1); div = QFrame(self); div.setObjectName("hline"); div.setFixedWidth(1); root.addWidget(nav); root.addWidget(div); root.addWidget(self.stack, 1)
        for build in (self.b_general, self.b_appearance, self.b_personal, self.b_models, self.b_api, self.b_hardware, self.b_chat, self.b_account, self.b_about): build()
        self.grp.idClicked.connect(lambda i: self.go(i, True)); self.go(max(0, min(8, page)), False)

    def showEvent(self, e):
        super().showEvent(e)
        if Cfg.anim() and not self._shown:
            self._shown = True; self.setWindowOpacity(0.0); a = QPropertyAnimation(self, b"windowOpacity", self); a.setDuration(180); a.setStartValue(0.0); a.setEndValue(1.0); a.start(); self._a = a

    def go(self, i, animate):
        self.stack.setCurrentIndex(i); b = self.grp.button(i)
        if b: b.setChecked(True)
        if animate: fade(self.stack.currentWidget(), True, 190)

    def page(self, title, sub=""):
        sa = QScrollArea(); sa.setWidgetResizable(True); sa.setFrameShape(QFrame.NoFrame); sa.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        w = QWidget(); l = vbox(w, 0, 6); l.setContentsMargins(36, 30, 36, 30); l.addWidget(label(title, "h1"))
        if sub: s = label(sub, "muted"); s.setWordWrap(True); l.addWidget(s)
        l.addSpacing(10); sa.setWidget(w); self.stack.addWidget(sa); return l

    # 1) Genel
    def b_general(self):
        l = self.page("Genel", "Uygulamanın genel davranışları. Değişiklikler hemen kaydedilir.")
        lang = QComboBox(); lang.addItems(["Otomatik (modelin kararı)", "Türkçe", "English"]); lang.setFixedWidth(240); lang.setCurrentIndex({"tr": 1, "en": 2}.get(Cfg.get("lang", "auto"), 0))
        lang.currentIndexChanged.connect(lambda i: Cfg.set("lang", ["auto", "tr", "en"][i])); l.addWidget(setting_row("Yanıt dili", "Seçim, sistem istemiyle modele iletilir. Arayüz dili şu an Türkçe'dir.", lang)); l.addWidget(hline())
        st = QComboBox(); st.addItems(["Yeni sohbetle başla", "Son sohbeti aç"]); st.setFixedWidth(240); st.setCurrentIndex(int(Cfg.get("startup", 0))); st.currentIndexChanged.connect(lambda i: Cfg.set("startup", i))
        l.addWidget(setting_row("Başlangıç davranışı", "Uygulama açıldığında ne gösterileceği.", st)); l.addWidget(hline())
        keep = QCheckBox(); keep.setChecked(bool(Cfg.get("keepSignedIn", True))); keep.toggled.connect(lambda b: Cfg.set("keepSignedIn", b))
        l.addWidget(setting_row("Oturumu açık tut", "Bu bilgisayarda son kullanıcı otomatik açılır. Parola saklanmaz; yalnızca hesap kimliği hatırlanır.", keep)); l.addWidget(hline())
        an = QCheckBox(); an.setChecked(Cfg.anim()); an.toggled.connect(lambda b: Cfg.set("anim", b)); l.addWidget(setting_row("Animasyonlar", "Kapatıldığında geçişler ve açılır pencere animasyonları devre dışı kalır.", an)); l.addStretch(1)

    # 2) Görünüm
    def b_appearance(self):
        l = self.page("Görünüm", "Tema seçimi kaydedilir ve yeniden başlatmada hatırlanır."); row = QWidget(); h = hbox(row, 0, 16); g = QButtonGroup(self); cur = int(Cfg.get("theme", 0))
        for i, n in enumerate(("Açık", "Koyu", "Sistem")): c = ThemeCard(i, n, row); c.setChecked(i == cur); g.addButton(c, i); h.addWidget(c)
        h.addStretch(1); g.idClicked.connect(lambda i: (Cfg.set("theme", i), switch_theme(self))); l.addSpacing(8); l.addWidget(row); l.addStretch(1)

    # 3) Kişiselleştirme
    def b_personal(self):
        l = self.page("Kişiselleştirme", "Görünen adınız, avatar baş harfleriniz ve modele iletilecek özel talimatlar."); u = Users.cur
        self.avatar = AvatarWidget(64); self.avatar.set_initials(u.initials); top = QWidget(); th = hbox(top, 0, 16); th.addWidget(self.avatar); tv = QVBoxLayout(); tv.addWidget(QLabel(u.full_name)); tv.addWidget(label(u.email, "small")); th.addLayout(tv); th.addStretch(1); l.addWidget(top)
        self.p_first, self.p_last = text_field("Ad", u.first), text_field("Soyad", u.last); self.p_ini = text_field("Otomatik", "" if initials_of(u.first, u.last) == u.initials else u.initials)
        self.p_ini.setMaxLength(2); self.p_ini.setFixedWidth(90); self.p_first.setFixedWidth(240); self.p_last.setFixedWidth(240)
        upd = lambda: self.avatar.set_initials(self.p_ini.text().upper() if self.p_ini.text() else initials_of(self.p_first.text(), self.p_last.text()))
        for w in (self.p_ini, self.p_first, self.p_last): w.textChanged.connect(upd)
        l.addWidget(setting_row("Ad", "", self.p_first)); l.addWidget(hline()); l.addWidget(setting_row("Soyad", "", self.p_last)); l.addWidget(hline())
        l.addWidget(setting_row("Avatar baş harfleri", "Boş bırakırsanız addan ve soyaddan otomatik üretilir (en fazla 2 harf).", self.p_ini)); l.addWidget(hline()); l.addWidget(label("Özel talimatlar", "h2"))
        d = label("Her sohbette sistem istemi olarak modele gönderilir. Hassas bilgi yazmayın.", "small"); d.setWordWrap(True); l.addWidget(d)
        self.p_ins = QTextEdit(); self.p_ins.setObjectName("field"); self.p_ins.setAcceptRichText(False); self.p_ins.setPlainText(u.instructions); self.p_ins.setMinimumHeight(120); self.p_ins.setMaximumHeight(180); l.addWidget(self.p_ins)
        self.p_status = QLabel(); save = pbtn("Kaydet", "primary"); save.setFixedWidth(120)

        def do_save():
            cu = Users.cur; e = Users.update_profile(self.p_first.text(), self.p_last.text(), cu.username, cu.email, self.p_ini.text()) or Users.set_instructions(self.p_ins.toPlainText()); set_status(self.p_status, e or "Kaydedildi.", 2 if e else 1)
        save.clicked.connect(do_save); l.addWidget(save); l.addWidget(self.p_status); l.addStretch(1)

    # 4) Modeller
    def b_models(self):
        l = self.page("Modeller", "Model listesi, etkin (varsayılan) model, bellek tahmini ve uyumluluk. Bellek değerleri tahmindir; bilinmeyenler “Bilinmiyor” olarak gösterilir.")
        row = QWidget(); rh = hbox(row, 0, 20); self.m_list = QListWidget(); self.m_list.setFixedWidth(290); self.m_list.setMinimumHeight(430); right = QWidget(); rv = vbox(right, 0, 12)
        self.m_detail = QLabel(); self.m_detail.setWordWrap(True); self.m_detail.setTextFormat(Qt.RichText); self.m_detail.setAlignment(Qt.AlignTop | Qt.AlignLeft); self.m_detail.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Minimum)
        rv.addWidget(self.m_detail); rv.addWidget(label("Anlık bellek", "small")); rv.addWidget(HardwarePanel(True)); br = QWidget(); bh = hbox(br, 0, 8)
        self.m_act, add, self.m_edit, self.m_del = pbtn("Etkin model yap", "primary"), pbtn("Özel model ekle…"), pbtn("Düzenle…"), pbtn("Sil", "danger")
        for b in (self.m_act, add, self.m_edit, self.m_del): bh.addWidget(b)
        bh.addStretch(1); rv.addWidget(br); rv.addStretch(1); rh.addWidget(self.m_list); rh.addWidget(right, 1); l.addWidget(row); l.addStretch(1)
        self.m_list.currentRowChanged.connect(lambda _: self.show_model()); self.m_act.clicked.connect(lambda: CAT.set_active(self.m_list.currentItem().data(Qt.UserRole)) if self.m_list.currentItem() else None)
        add.clicked.connect(self._m_add); self.m_edit.clicked.connect(self._m_edit); self.m_del.clicked.connect(self._m_del); CAT.changed.connect(self.refresh_models); self.refresh_models()

    def _m_add(self):
        d = ModelEditDialog(ModelEntry(), True, self)
        if d.exec() == QDialog.Accepted: self.refresh_models(CAT.add_custom(d.result_entry()))

    def _m_edit(self):
        it = self.m_list.currentItem()
        if not it: return
        e = CAT.by_id(it.data(Qt.UserRole))
        if e.builtin or e.id.startswith("cfg:"): return
        d = ModelEditDialog(e, False, self)
        if d.exec() == QDialog.Accepted: CAT.update_custom(d.result_entry())

    def _m_del(self):
        it = self.m_list.currentItem()
        if not it: return
        e = CAT.by_id(it.data(Qt.UserRole))
        if not e.builtin and not e.id.startswith("cfg:") and confirm(self, "Modeli sil", f"“{e.name}” özel modeli silinsin mi?"): CAT.remove_custom(e.id)

    def refresh_models(self, select=""):
        keep = select or (self.m_list.currentItem().data(Qt.UserRole) if self.m_list.currentItem() else ""); act = CAT.active_id(); self.m_list.blockSignals(True); self.m_list.clear(); sel = 0
        for i, e in enumerate(CAT.all()):
            it = QListWidgetItem(f"{e.name}{'   ★ etkin' if e.id == act else ''}\n{e.family} · {e.location()}"); it.setData(Qt.UserRole, e.id); self.m_list.addItem(it)
            if e.id == keep: sel = i
        self.m_list.setCurrentRow(sel); self.m_list.blockSignals(False); self.show_model()

    def show_model(self):
        it = self.m_list.currentItem()
        if not it: return
        e = CAT.by_id(it.data(Qt.UserRole)); self.m_detail.setText(model_detail_html(e)); custom = not e.builtin and not e.id.startswith("cfg:")
        self.m_edit.setEnabled(custom); self.m_del.setEnabled(custom); self.m_act.setEnabled(e.id != CAT.active_id())

    # 5) API ve Bağlantılar
    def b_api(self):
        l = self.page("API ve Bağlantılar", "Sağlayıcıları yapılandırın ve bağlantıyı gerçek bir istekle test edin. API anahtarları Windows Kimlik Bilgisi Yöneticisi'nde saklanır (keyring); günlüklere yazılmaz.")
        seg = QWidget(); sh = hbox(seg, 0, 6); sg = QButtonGroup(self); sg.setExclusive(True)
        b1 = IBtn("key", " Bulut / API sağlayıcıları", "", 18, "text", seg); b2 = IBtn("server", " RÜZGAR Local Server", "", 18, "text", seg)
        for i, b in enumerate((b1, b2)): b.setCheckable(True); b.setFixedHeight(38); sg.addButton(b, i); sh.addWidget(b)
        b1.setChecked(True); sh.addStretch(1); self.api_stack = QStackedWidget(); self.api_stack.addWidget(self._cloud()); self.api_stack.addWidget(self._local())
        sg.idClicked.connect(lambda i: (self.api_stack.setCurrentIndex(i), fade(self.api_stack.currentWidget(), True, 160))); l.addWidget(seg); l.addSpacing(6); l.addWidget(self.api_stack); l.addStretch(1)

    def _cloud(self):
        w = QWidget(); h = hbox(w, 0, 22); left = QWidget(); left.setFixedWidth(250); lv = vbox(left, 0, 8); self.prov_list = QListWidget(); self.prov_list.setMinimumHeight(420)
        add_btn = IBtn("plus", " Sağlayıcı ekle", "", 18, "text", left); add_btn.setFixedHeight(38); lv.addWidget(self.prov_list, 1); lv.addWidget(add_btn)
        form = QWidget(); g = QGridLayout(form); g.setContentsMargins(0, 0, 0, 0); g.setHorizontalSpacing(14); g.setVerticalSpacing(10); g.setColumnStretch(1, 1); r = [0]

        def add(lbl, f):
            ll = label(lbl, "muted"); g.addWidget(ll, r[0], 0); g.addWidget(f, r[0], 1); r[0] += 1; return ll
        self.f_name = text_field("Görünen ad"); add("Sağlayıcı adı", self.f_name); self.f_kind = QComboBox()
        for k in CLOUD_KINDS: self.f_kind.addItem(PK_NAMES[k], k)
        add("Sağlayıcı türü", self.f_kind); self.f_url = text_field("https://…/v1"); add("API Base URL", self.f_url); self.f_key = pw_field("API anahtarı"); add("API anahtarı", self.f_key)
        self.f_model = QComboBox(); self.f_model.setEditable(True); self.f_model.setInsertPolicy(QComboBox.NoInsert); add("Model kimliği", self.f_model); self.f_org = text_field("İsteğe bağlı"); self.org_lbl = add("Kuruluş kimliği", self.f_org)
        self.f_timeout = num_spin(5, 900, 60); add("İstek zaman aşımı (sn)", self.f_timeout); self.f_vision = QComboBox(); self.f_vision.addItems(["Otomatik", "Evet (görsel destekli)", "Hayır"]); add("Görsel desteği", self.f_vision)
        self.f_hint = label("", "small"); self.f_hint.setWordWrap(True); g.addWidget(self.f_hint, r[0], 1); r[0] += 1
        br = QWidget(); bh = hbox(br, 0, 8); save = pbtn("Kaydet", "primary"); self.f_test = pbtn("Bağlantıyı test et"); self.f_def = pbtn("Varsayılan yap"); self.f_del = pbtn("Sil", "danger")
        for b in (save, self.f_test, self.f_def, self.f_del): bh.addWidget(b)
        bh.addStretch(1); g.addWidget(br, r[0], 1); r[0] += 1; self.f_status = QLabel(); set_status(self.f_status, "Durum: test edilmedi", 0); g.addWidget(self.f_status, r[0], 1); h.addWidget(left); h.addWidget(form, 1)

        def menu():
            m = QMenu(self)
            for k in CLOUD_KINDS: m.addAction(PK_NAMES[k]).setData(k)
            a = m.exec(add_btn.mapToGlobal(QPoint(0, add_btn.height() + 4)))
            if a: self.new_provider(a.data())
        add_btn.clicked.connect(menu); self.f_kind.currentIndexChanged.connect(self.on_kind); self.prov_list.currentRowChanged.connect(lambda row: None if self.loading or row < 0 else self.select_provider(self.prov_list.item(row).data(Qt.UserRole)))
        save.clicked.connect(self.save_provider); self.f_test.clicked.connect(self.test_provider); API.test_done.connect(self.on_test_done)
        self.f_def.clicked.connect(lambda: API.set_default(self.cur_id) if self.cur_id else None); self.f_del.clicked.connect(self.del_provider); API.changed.connect(lambda: self.refresh_providers(False)); self.refresh_providers()
        return w

    def refresh_providers(self, reselect=True):
        self.loading = True; self.prov_list.clear(); sel = -1
        for i, c in enumerate(API.provs):
            it = QListWidgetItem(f"{c.name}\n{PK_NAMES[c.kind]}{' · varsayılan' if c.id == API.default else ''}"); it.setData(Qt.UserRole, c.id); self.prov_list.addItem(it)
            if c.id == self.cur_id: sel = i
        if sel >= 0: self.prov_list.setCurrentRow(sel)
        self.loading = False
        if reselect:
            if sel >= 0: self.select_provider(self.cur_id)
            elif self.prov_list.count(): self.prov_list.setCurrentRow(0); self.select_provider(self.prov_list.item(0).data(Qt.UserRole))
            else: self.new_provider(OPENAI)
        self.f_def.setEnabled(bool(self.cur_id) and API.default != self.cur_id)

    def load_form(self, c, key):
        self.loading = True; self.f_name.setText(c.name); self.f_kind.setCurrentIndex(max(0, self.f_kind.findData(c.kind))); self.last_kind = c.kind; self.f_url.setText(c.base_url); self.f_key.setText(key); self.stored_key = key
        self.f_model.clear(); self.f_model.setEditText(c.model); self.f_org.setText(c.org); self.f_timeout.setValue(c.timeout); self.f_vision.setCurrentIndex(0 if c.vision < 0 else (1 if c.vision == 1 else 2)); self.loading = False; self.kind_ui()

    def select_provider(self, pid):
        c = API.find(pid)
        if not c: return
        self.cur_id = pid; self.load_form(c, API.key(pid)); self.f_def.setEnabled(API.default != pid)
        if c.last_tested: set_status(self.f_status, f"Son test ({time.strftime('%d.%m.%Y %H:%M', time.localtime(c.last_tested / 1000))}): {c.last_msg}", 1 if c.last_ok else 2)
        else: set_status(self.f_status, "Durum: test edilmedi", 0)

    def new_provider(self, k):
        self.cur_id = ""; self.load_form(Prov("", PK_NAMES[k], k, PK_URL.get(k, ""), PK_MODEL.get(k, "")), ""); self.prov_list.clearSelection(); self.f_def.setEnabled(False); set_status(self.f_status, "Yeni yapılandırma — kaydetmeyi unutmayın.", 0)

    def form_cfg(self):
        return Prov(self.cur_id, self.f_name.text().strip(), self.f_kind.currentData(), self.f_url.text().strip(), self.f_model.currentText().strip(), self.f_org.text().strip(), self.f_timeout.value(), [-1, 1, 0][self.f_vision.currentIndex()])

    def save_provider(self):
        c = self.form_cfg(); c.id = c.id or new_id(); key = self.f_key.text(); e = API.save(c, key, key != self.stored_key)
        if e: set_status(self.f_status, e, 2); return
        self.cur_id = c.id; self.stored_key = key; set_status(self.f_status, "Kaydedildi.", 1); self.refresh_providers(False)
        for i in range(self.prov_list.count()):
            if self.prov_list.item(i).data(Qt.UserRole) == self.cur_id: self.loading = True; self.prov_list.setCurrentRow(i); self.loading = False

    def test_provider(self):
        c = self.form_cfg(); self.test_id = c.id; self.f_test.setEnabled(False); set_status(self.f_status, "Test ediliyor…", 0); API.test(c, self.f_key.text())

    def on_test_done(self, pid, ok, msg, models):
        if pid != self.test_id: return
        self.f_test.setEnabled(True); set_status(self.f_status, "Durum: " + msg, 1 if ok else 2)
        if ok and models: cur = self.f_model.currentText(); self.f_model.clear(); self.f_model.addItems(models); self.f_model.setEditText(cur or models[0])

    def del_provider(self):
        if not self.cur_id: self.new_provider(OPENAI); return
        if confirm(self, "Sağlayıcıyı sil", f"“{self.f_name.text()}” yapılandırması ve kayıtlı API anahtarı silinsin mi?"): API.remove(self.cur_id); self.cur_id = ""; self.refresh_providers()

    def on_kind(self):
        if self.loading: return
        nk = self.f_kind.currentData()
        if not self.f_url.text().strip() or self.f_url.text().strip() == PK_URL.get(self.last_kind, ""): self.f_url.setText(PK_URL.get(nk, ""))
        if not self.f_model.currentText().strip() or self.f_model.currentText().strip() == PK_MODEL.get(self.last_kind, ""): self.f_model.setEditText(PK_MODEL.get(nk, ""))
        if not self.f_name.text().strip() or self.f_name.text().strip() == PK_NAMES[self.last_kind]: self.f_name.setText(PK_NAMES[nk])
        self.last_kind = nk; self.kind_ui()

    def kind_ui(self):
        k = self.f_kind.currentData(); org = k in (OPENAI, COMPAT); self.f_org.setVisible(org); self.org_lbl.setVisible(org)
        h = {OPENAI: "OpenAI Chat Completions uç noktası. Düşünme modları yalnızca desteklenen akıl yürütme modellerinde (reasoning_effort) iletilir.",
             ANTHROPIC: "Anthropic Messages API (x-api-key). Düşünme modları, Claude modellerinde genişletilmiş düşünme bütçesi olarak iletilir.",
             DEEPSEEK: "DeepSeek, OpenAI uyumlu sohbet uç noktası kullanır. Görsel girdisi varsayılan olarak desteklenmiyor kabul edilir.",
             GEMINI: "Google Gemini generateContent (akışlı) uç noktası. Düşünme bütçesi yalnızca Gemini 2.5+ modellerinde iletilir.",
             COMPAT: "OpenAI uyumlu herhangi bir sunucu (LM Studio, vLLM, OpenRouter vb.). Görsel desteği bilinmediği için istek sunucuya gönderilir; sunucu reddederse hata gösterilir."}.get(k, "Özel uç nokta, OpenAI sohbet biçimini (chat/completions) kullanmalıdır. /models uç noktası yoksa bağlantı testi hata verebilir.")
        self.f_hint.setText(h + " Model kimliğini sağlayıcının güncel belgelerinden doğrulayın.")

    def _local(self):
        w = QWidget(); g = QGridLayout(w); g.setContentsMargins(0, 0, 0, 0); g.setHorizontalSpacing(14); g.setVerticalSpacing(10); g.setColumnStretch(1, 1); r = [0]
        info = label("Bu bağlantı bulut sağlayıcılarından ayrıdır. Sunucu OpenAI uyumlu /v1/chat/completions ve /v1/models uç noktalarını sunmalıdır (örn. gelecekteki RÜZGAR Python sunucusu).", "muted"); info.setWordWrap(True); g.addWidget(info, 0, 0, 1, 2); r[0] = 1

        def add(lbl, f): g.addWidget(label(lbl, "muted"), r[0], 0); g.addWidget(f, r[0], 1); r[0] += 1
        self.l_addr = text_field("http://127.0.0.1", LocalServer.address()); add("Sunucu adresi", self.l_addr); self.l_port = num_spin(1, 65535, LocalServer.port()); add("Bağlantı noktası", self.l_port)
        self.l_key = pw_field("İsteğe bağlı API anahtarı"); self.l_key.setText(LocalServer.key()); add("API anahtarı", self.l_key)
        self.l_model = QComboBox(); self.l_model.setEditable(True); self.l_model.setInsertPolicy(QComboBox.NoInsert); self.l_model.addItems(LocalServer.models()); self.l_model.setEditText(LocalServer.model()); add("Seçili model", self.l_model)
        self.l_models = label("", "small"); add("Kullanılabilir modeller", self.l_models); br = QWidget(); bh = hbox(br, 0, 8); save = pbtn("Kaydet", "primary"); self.l_test = pbtn("Bağlantıyı test et"); bh.addWidget(save); bh.addWidget(self.l_test); bh.addStretch(1)
        g.addWidget(br, r[0], 1); r[0] += 1; self.l_status = QLabel(); g.addWidget(self.l_status, r[0], 1); g.setRowStretch(r[0] + 1, 1); self.upd_local()

        def do_save():
            e = LocalServer.save(self.l_addr.text(), self.l_port.value(), self.l_key.text(), self.l_model.currentText()); set_status(self.l_status, e or "Kaydedildi.", 2 if e else 1)
        save.clicked.connect(do_save); self.l_test.clicked.connect(lambda: (self.l_test.setEnabled(False), set_status(self.l_status, "Test ediliyor…", 0), LocalServer.test(self.l_addr.text(), self.l_port.value(), self.l_key.text())))
        LocalServer.inst.test_done.connect(self.on_local_test); return w

    def on_local_test(self, ok, msg, models):
        self.l_test.setEnabled(True); set_status(self.l_status, "Sunucu durumu: " + ("Bağlı" if ok else "Bağlı değil") + "\n" + msg, 1 if ok else 2)
        if ok: cur = self.l_model.currentText(); self.l_model.clear(); self.l_model.addItems(models); self.l_model.setEditText(cur or (models[0] if models else ""))
        self.upd_local()

    def upd_local(self):
        ms = LocalServer.models(); self.l_models.setText("Henüz listelenmedi (bağlantıyı test edin)" if not ms else f"{len(ms)} model: " + ", ".join(ms[:6]) + (" …" if len(ms) > 6 else ""))

    # 6) Donanım
    def b_hardware(self): l = self.page("Donanım", "Canlı CPU, RAM ve (NVIDIA için) VRAM bilgisi. Okunamayan değerler uydurulmaz."); l.addSpacing(8); l.addWidget(HardwarePanel(False)); l.addStretch(1)

    # 7) Sohbet
    def b_chat(self):
        l = self.page("Sohbet", "Mesaj gönderme ve sohbet geçmişi ayarları.")
        en = QComboBox(); en.addItems(["Enter ile gönder (Shift+Enter: yeni satır)", "Ctrl+Enter ile gönder (Enter: yeni satır)"]); en.setFixedWidth(330); en.setCurrentIndex(0 if Cfg.get("enterSends", True) else 1)
        en.currentIndexChanged.connect(lambda i: Cfg.set("enterSends", i == 0)); l.addWidget(setting_row("Enter davranışı", "", en)); l.addWidget(hline())
        sc = QCheckBox(); sc.setChecked(bool(Cfg.get("streamUi", True))); sc.toggled.connect(lambda b: Cfg.set("streamUi", b))
        l.addWidget(setting_row("Yanıtı akış halinde göster", "Kapalıyken yanıt tamamlanana kadar yazıyor göstergesi görünür, sonra metin bir kerede gösterilir.", sc)); l.addWidget(hline())
        hs = num_spin(0, 500, Cfg.get("maxHist", 0)); hs.valueChanged.connect(lambda v: Cfg.set("maxHist", v)); l.addWidget(setting_row("Gönderilecek son mesaj sayısı", "0 = tüm sohbet geçmişi gönderilir. Düşük değerler maliyeti ve gecikmeyi azaltır.", hs)); l.addWidget(hline())
        mt = num_spin(256, 64000, Cfg.get("maxTokens", 4096)); mt.valueChanged.connect(lambda v: Cfg.set("maxTokens", v)); l.addWidget(setting_row("Anthropic için en fazla yanıt token'ı", "Anthropic Messages API bu değeri zorunlu tutar; diğer sağlayıcılara gönderilmez.", mt)); l.addWidget(hline())
        ex = pbtn("JSON olarak dışa aktar…"); de = pbtn("Tüm sohbetleri sil", "danger")

        def export():
            p, _ = QFileDialog.getSaveFileName(self, "Sohbetleri dışa aktar", str(Path.home() / "ruzgar-sohbetler.json"), "JSON (*.json)")
            if not p: return
            try: Path(p).write_text(json.dumps(Convs.export_all(), ensure_ascii=False, indent=2), "utf-8")
            except OSError as e: QMessageBox.warning(self, "Hata", f"Dosya yazılamadı: {e}")

        def wipe():
            if confirm(self, "Tüm sohbetleri sil", "Bu hesaptaki TÜM sohbetler kalıcı olarak silinsin mi?"): Convs.delete_all(); self.cleared = True
        ex.clicked.connect(export); de.clicked.connect(wipe); l.addWidget(setting_row("Sohbetleri dışa aktar", "Tüm sohbetlerinizi tek bir JSON dosyasına kaydeder.", ex)); l.addWidget(hline()); l.addWidget(setting_row("Tüm sohbetleri sil", "Bu işlem geri alınamaz.", de)); l.addStretch(1)

    # 8) Hesap
    def b_account(self):
        l = self.page("Hesap", "Bu bir yerel hesaptır: yalnızca bu bilgisayarda saklanır, cihazlar arasında eşitlenmez."); u = Users.cur
        self.a_user, self.a_mail = text_field("Kullanıcı adı", u.username), text_field("E-posta", u.email); self.a_user.setFixedWidth(280); self.a_mail.setFixedWidth(280)
        l.addWidget(setting_row("Kullanıcı adı", "", self.a_user)); l.addWidget(hline()); l.addWidget(setting_row("E-posta", "", self.a_mail)); self.a_status = QLabel(); sp = pbtn("Profili kaydet", "primary"); sp.setFixedWidth(150)

        def save():
            cu = Users.cur; e = Users.update_profile(cu.first, cu.last, self.a_user.text(), self.a_mail.text(), "" if initials_of(cu.first, cu.last) == cu.initials else cu.initials); set_status(self.a_status, e or "Profil kaydedildi.", 2 if e else 1)
        sp.clicked.connect(save); l.addWidget(sp); l.addWidget(self.a_status); l.addSpacing(10); l.addWidget(hline()); l.addWidget(label("Parolayı değiştir", "h2"))
        o, n1, n2 = pw_field("Mevcut parola"), pw_field("Yeni parola (en az 8 karakter)"), pw_field("Yeni parolayı doğrula"); pst = QLabel(); cp = pbtn("Parolayı değiştir"); cp.setFixedWidth(170)
        for w in (o, n1, n2): w.setMaximumWidth(380)

        def chpw():
            e = Users.change_password(o.text(), n1.text(), n2.text()); set_status(pst, e or "Parola değiştirildi.", 2 if e else 1)
            if not e: o.clear(); n1.clear(); n2.clear()
        cp.clicked.connect(chpw)
        for w in (o, n1, n2, cp, pst): l.addWidget(w)
        l.addSpacing(10); l.addWidget(hline()); so, da = pbtn("Çıkış yap"), pbtn("Yerel hesabı sil", "danger")

        def out(): self.sign_out = True; self.accept()

        def delete():
            pw = ask_text(self, "Hesabı sil", "Hesabınız ve tüm sohbetleriniz kalıcı olarak silinecek. Onaylamak için parolanızı girin:", "", True)
            if pw is None: return
            e = Users.delete_account(pw)
            if e: QMessageBox.warning(self, "Hesap silinemedi", e); return
            self.sign_out = True; self.accept()
        so.clicked.connect(out); da.clicked.connect(delete)
        l.addWidget(setting_row("Çıkış yap", "Oturumu kapatır; hesabınız ve sohbetleriniz bu bilgisayarda kalır.", so)); l.addWidget(hline())
        l.addWidget(setting_row("Yerel hesabı sil", "Hesabı ve tüm sohbetleri kalıcı olarak siler. API anahtarları ayrıca silinmez.", da)); l.addStretch(1)

    # 9) Hakkında
    def b_about(self):
        l = self.page("RÜZGAR AI Hakkında"); top = QWidget(); th = hbox(top, 0, 14); th.addWidget(LogoLabel(56)); tv = QVBoxLayout(); tv.addWidget(label("RÜZGAR AI", "h2")); import PySide6
        tv.addWidget(label(f"Sürüm 1.0 (Python) · PySide6 {PySide6.__version__} · Qt {qVersion()}", "muted")); th.addLayout(tv); th.addStretch(1); l.addWidget(top); l.addSpacing(8)
        txt = (f"• Hesaplar ve sohbetler yerel SQLite veritabanında saklanır: {APP_DIR}\n• Parolalar tuzlanmış PBKDF2-HMAC-SHA256 ile özetlenir; düz metin saklanmaz.\n"
               "• API anahtarları Windows Kimlik Bilgisi Yöneticisi'nde (keyring) tutulur; keyring yoksa yalnızca oturum boyunca bellekte kalır.\n• Hesaplar cihazlar arasında eşitlenmez.\n"
               "• Ses tanıma bu sürümde yoktur (mikrofon düğmesi yalnızca arayüz öğesidir).\n• GPU/VRAM bilgisi yalnızca NVIDIA (nvidia-smi) için okunur.\n• Model bellek değerleri tahmindir; gerçek ölçüm değildir.\n• Yapay zekâ yanıtlarındaki kod hiçbir zaman çalıştırılmaz.")
        t = QLabel(txt); t.setWordWrap(True); t.setTextInteractionFlags(Qt.TextSelectableByMouse); l.addWidget(t); l.addStretch(1)


# ============================================================================
# 13) Görseller / bilgi sayfaları, ana pencere, giriş noktası
# ============================================================================
class ImagesPage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent); v = vbox(self); hd = label("Görseller", "h1", self); hd.setContentsMargins(36, 22, 0, 4); sub = label("Sohbetlerinize eklediğiniz görseller. Açmak için tıklayın.", "muted", self); sub.setContentsMargins(36, 0, 0, 10)
        self.scroll = SmoothScroll(self); self.host = QWidget(); self.grid = QGridLayout(self.host); self.grid.setContentsMargins(36, 6, 36, 30); self.grid.setSpacing(12); self.grid.setAlignment(Qt.AlignTop | Qt.AlignLeft); self.scroll.setWidget(self.host)
        self.empty = label("Henüz görsel eklemediniz. Mesaj alanındaki + düğmesiyle eklediğiniz görseller burada görünür.", "muted", self); self.empty.setAlignment(Qt.AlignCenter); self.empty.setWordWrap(True)
        for w, s in ((hd, 0), (sub, 0), (self.scroll, 1), (self.empty, 1)): v.addWidget(w, s)
        self.items = []; self.cols = 0

    def reload(self): self.items = Convs.all_images(); self.rebuild()
    def _cols(self): return max(1, (self.width() - 72 + 12) // 172)

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if self._cols() != self.cols: self.rebuild()

    def rebuild(self):
        while self.grid.count(): it = self.grid.takeAt(0); it.widget() and it.widget().deleteLater()
        self.cols = self._cols()
        for i, (path, _cid) in enumerate(self.items):
            ex = Path(path).exists(); ri = RoundImage(thumb_pix(path, 160) if ex else None, 160, 14, Path(path).name if ex else "Dosya bulunamadı", self.host)
            if ex: ri.set_clickable(True); ri.setToolTip(Path(path).name); ri.clicked.connect(lambda p=path: QDesktopServices.openUrl(QUrl.fromLocalFile(p)))
            self.grid.addWidget(ri, i // self.cols, i % self.cols)
        self.scroll.setVisible(bool(self.items)); self.empty.setVisible(not self.items)


class InfoPage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent); v = vbox(self); v.setContentsMargins(36, 22, 36, 30); self.t = label("", "h1", self); self.x = label("", "muted", self); self.x.setWordWrap(True)
        v.addWidget(self.t); v.addSpacing(8); v.addWidget(self.x); v.addStretch(1)

    def set_content(self, t, x): self.t.setText(t); self.x.setText(x)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__(); self.setWindowTitle("RÜZGAR AI"); self.resize(1365, 768); self.setMinimumSize(900, 600)
        g = Cfg.get("geometry")
        if g: self.restoreGeometry(QByteArray.fromBase64(g.encode()))
        self.root = QStackedWidget(self); self.setCentralWidget(self.root); self.auth = AuthScreen(); self.shell = QWidget(); self.root.addWidget(self.auth); self.root.addWidget(self.shell)
        h = hbox(self.shell); self.sb = Sidebar(self.shell); self.pages = QStackedWidget(self.shell); self.chat = ChatWidget(); self.images = ImagesPage(); self.info = InfoPage()
        for w in (self.chat, self.images, self.info): self.pages.addWidget(w)
        h.addWidget(self.sb); h.addWidget(self.pages, 1)
        self.auth.authenticated.connect(self.enter_app)
        self.sb.new_chat.connect(lambda: (self.chat.new_conversation(), self.sb.set_active_conv(""), self.show_chat()))
        self.sb.open_conv.connect(lambda cid: (self.show_chat(), self.chat.open_conversation(cid), self.sb.set_active_conv(cid)))
        self.sb.conv_deleted.connect(lambda cid: (self.chat.new_conversation(), self.sb.set_active_conv("")) if self.chat.conv_id == cid else None)
        self.sb.section_requested.connect(self.open_section); self.sb.account_action.connect(self.account_action); self.sb.collapse_requested.connect(self.toggle_sidebar)
        self.sb.open_changed.connect(self.chat.header.set_sidebar_open); self.chat.conversation_activated.connect(self.sb.set_active_conv)
        self.chat.open_settings_requested.connect(self.open_settings); self.chat.toggle_sidebar_requested.connect(self.toggle_sidebar)
        self.chat.header.model.add_requested.connect(self.add_model); self.chat.header.model.manage_requested.connect(lambda: self.open_settings(3)); BUS.error.connect(lambda m: toast(self, m, True))
        self.chat.header.set_sidebar_open(self.sb.open)
        for seq, fn in (("Ctrl+N", lambda: (self.chat.new_conversation(), self.sb.set_active_conv(""), self.show_chat())), ("Ctrl+K", self.sb.focus_search), ("Ctrl+B", self.toggle_sidebar), ("Ctrl+,", lambda: self.open_settings(0))):
            sc = QShortcut(QKeySequence(seq), self); sc.activated.connect(lambda fn=fn: fn() if self.root.currentWidget() is self.shell else None)
        try: QGuiApplication.styleHints().colorSchemeChanged.connect(lambda *_: switch_theme(self) if Cfg.get("theme", 0) == 2 else None)
        except Exception: pass
        if Users.auto_login(): self.enter_app()
        else: self.auth.start(); self.root.setCurrentWidget(self.auth)
        self.sb.set_full_width(280 if self.width() < 1100 else 340)

    def resizeEvent(self, e): super().resizeEvent(e); self.sb.set_full_width(280 if self.width() < 1100 else 340)

    def closeEvent(self, e):
        Cfg.set("geometry", bytes(self.saveGeometry().toBase64()).decode()); self.chat.stop_generation()
        for j in list(JOBS): j.abort(); j.wait(1500)
        super().closeEvent(e)

    def show_chat(self): self.pages.setCurrentWidget(self.chat); self.sb.set_active_section(""); self.chat.comp.focus_input()
    def toggle_sidebar(self): self.sb.set_open(not self.sb.open, True)
    def add_model(self):
        d = ModelEditDialog(ModelEntry(), True, self)
        if d.exec() == QDialog.Accepted: CAT.set_active(CAT.add_custom(d.result_entry()))

    def enter_app(self):
        self.root.setCurrentWidget(self.shell); self.sb.update_user(); self.sb.refresh()
        if int(Cfg.get("startup", 0)) == 1:
            cid = Convs.last_id()
            if cid: self.chat.open_conversation(cid); self.sb.set_active_conv(cid); self.show_chat(); fade(self.shell, True, 260); return
        self.chat.new_conversation(); self.sb.set_active_conv(""); self.show_chat(); fade(self.shell, True, 260)

    def sign_out(self):
        self.chat.stop_generation()
        if Users.logged_in(): Users.logout()
        self.pages.setCurrentWidget(self.chat); self.auth.start(); self.root.setCurrentWidget(self.auth); fade(self.auth, True, 240)

    def open_section(self, k):
        if k == "images": self.images.reload(); self.pages.setCurrentWidget(self.images)
        else:
            txt = {"tasks": ("Görevler", "Zamanlanmış görevler bu yerel sürümde henüz etkin değil."), "library": ("Kitaplık", "Kitaplık bu yerel sürümde henüz etkin değil. Sabitlediğiniz sohbetlere kenar çubuğundan ulaşabilirsiniz."),
                   "discover": ("Keşfet", "Keşfet bölümü bu yerel sürümde henüz etkin değil.")}[k]
            self.info.set_content(*txt); self.pages.setCurrentWidget(self.info)
        self.sb.set_active_section(k); fade(self.pages.currentWidget(), True, 200)

    def account_action(self, k):
        if k == "signout": self.sign_out(); return
        self.open_settings({"settings": 0, "appearance": 1, "models": 3, "api": 4, "hardware": 5, "account": 7}.get(k, 0))

    def open_settings(self, page):
        dlg = SettingsDialog(self, page); dlg.exec()
        if dlg.sign_out: self.sign_out(); return
        if dlg.cleared: self.chat.new_conversation(); self.sb.set_active_conv(""); self.show_chat()
        self.sb.update_user(); self.sb.refresh()


def main():
    global BUS, HW, API, CAT
    app = QApplication(sys.argv); app.setOrganizationName("RuzgarAI"); app.setApplicationName("RuzgarAI"); app.setApplicationDisplayName("RÜZGAR AI"); app.setStyle("Fusion")
    f = QFont("Segoe UI"); f.setPixelSize(14); app.setFont(f)
    BUS = Bus(); Cfg.load(); apply_theme(); app.setWindowIcon(QIcon(logo_pix(64)))
    try: Db.open()
    except sqlite3.Error as e: QMessageBox.critical(None, "Veritabanı hatası", f"Yerel veritabanı açılamadı:\n{e}"); return 1
    HW = Hardware(); API = Api(); LocalServer.inst = LocalServer(); CAT = Catalog()
    w = MainWindow(); w.show(); return app.exec()


if __name__ == "__main__":
    sys.exit(main())
