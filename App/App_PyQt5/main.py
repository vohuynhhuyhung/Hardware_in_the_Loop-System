# File: main.py
import sys
from PyQt5.QtWidgets import QApplication
from ui.main_window import MainWindow

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Global fixed font sizes cho UI labels, buttons, groupboxes...
    app.setStyleSheet("""
    QLabel, QPushButton, QGroupBox, QCheckBox, QComboBox, QTabWidget, QLineEdit {
        font-size: 10pt;
    }
    QTableWidget, QHeaderView::section {
        font-size: 10pt;
    }
    QTextEdit {
        font-family: Consolas;
        font-size: 8pt;
    }
    """)

    win = MainWindow()
    win.showMaximized()
    sys.exit(app.exec_())
