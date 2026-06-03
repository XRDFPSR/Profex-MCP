# -*- mode: python ; coding: utf-8 -*-
"""
pyprofex PyInstaller spec.
Build command:
    pyinstaller pyprofex/pyprofex.spec
"""
import os, sys

# Absolute paths
BASE_DIR = os.path.dirname(os.path.abspath(__file__)) if '__file__' in dir() else os.getcwd()
PYPROFEX_DIR = os.path.join(BASE_DIR, 'pyprofex')

# The pickle data file
DATA_PKL = os.path.join(PYPROFEX_DIR, 'fingerprints_unified.pkl')

block_cipher = None

a = Analysis(
    [os.path.join(PYPROFEX_DIR, 'profex_cli.py')],
    pathex=[BASE_DIR],
    binaries=[],
    datas=[
        (DATA_PKL, 'pyprofex'),
    ],
    hiddenimports=[
        'search_match',
        'cif2fingerprint',
        'peaks',
        'mcp_server',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[
        'tkinter', 'matplotlib', 'scipy', 'PIL', 'numpy',
        'Jinja2', 'sphinx', 'PyQt5', 'PyQt6', 'qtpy',
        'sqlite3', 'pandas', 'sympy', 'cv2',
    ],
    win_no_prefer_redirects=False,
    win_no_default_exclude_redirects=False,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='pyprofex',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
