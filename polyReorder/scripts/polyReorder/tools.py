"""
polyReorder 

Support script for polyReorder plugin.
"""

from maya import cmds 

def polyReorderTool(*args):
    cmds.setToolTo(cmds.polyReorderCtx())