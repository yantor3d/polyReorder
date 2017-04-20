"""polyReorder

The _initializePlugin and _uninitializePlugin functions are automatically 
called when the plugin is un/loaded to create the polyReorder tools menu.

"""

import polyReorder.tools as _tools

import maya.api.OpenMaya as OpenMaya
import maya.cmds as cmds


_POLY_REORDER_MENU_NAME = 'polyReorderMenu'   


class _MenuItem(object):
    """Simple wrapper to keep the script editor is pretty.
    
    Attributes
    ----------
    name : str
        Label for this menu item.
    action : callable
        Command executed when this menu item is clicked.
    isOptionBox : bool
        If True, this menu item is an option box.

    """

    def __init__(self, name, action=None, isOptionBox=False):
        self.name = name 
        self.action = action 
        self.isOptionBox = isOptionBox

    def __str__(self):
        return self.name.replace(' ', '')

    def __call__(self, *args, **kwargs):
        try:
            self.action()
        except Exception as e:
            OpenMaya.MGlobal.displayError(str(e).strip())


_POLY_REORDER_MENU_ITEMS = (
    _MenuItem('Poly Reorder Tool', _tools.polyReorderTool),
)


def _try_delete_menu():
    if cmds.menu(_POLY_REORDER_MENU_ITEMS, query=True, exists=True):
        cmds.deleteUI(_POLY_REORDER_MENU_ITEMS)   
        

def _initializePlugin(*args):
    """Construct the poly symmetry plugin menu."""
            
    if cmds.about(batch=True):
        return
        
    _try_delete_menu()
    
    cmds.menu(
        _POLY_REORDER_MENU_ITEMS,
        label="Poly Reorder", 
        parent='MayaWindow'
    )

    for item in _POLY_REORDER_MENU_ITEMS:
        _addMenuItem(item)

    _addMenuItem(
        _MenuItem('Reload Menu', _initializePlugin)
    )
        
    cmds.menuSet("riggingMenuSet", addMenu=_POLY_REORDER_MENU_NAME)
    

def _addMenuItem(item):
    if item is None:
        cmds.menuItem(divider=True)
    else:
        cmds.menuItem(
            label=item.name,
            command=item, 
            sourceType='python',
            optionBox=item.isOptionBox
        )


def _uninitializePlugin():
    """Construct the poly reorder plugin menu."""

    if cmds.about(batch=True):
        return
    
    _try_delete_menu()