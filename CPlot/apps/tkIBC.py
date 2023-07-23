# - IBC app -
"""Interface to set IBCs."""

try: import tkinter as TK
except: import Tkinter as TK
import CPlot.Ttk as TTK
import Converter.PyTree as C
import Geom.PyTree as D
import CPlot.PyTree as CPlot
import CPlot.Tk as CTK
import Converter.Internal as Internal
import CPlot.iconics as iconics

# local widgets list
WIDGETS = {}; VARS = []

# Set IBM data in zone
# if zone is 1D STRUCT or BAR: remesh
def _setDataInZone(z, snear, ibctype, dfar, inv):
    # set IBM data in .Solver#define
    n = Internal.createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
    Internal.createUniqueChild(n, 'snear', 'DataArray_t', value=snear)
    Internal.createUniqueChild(n, 'ibctype', 'DataArray_t', value=ibctype)
    Internal.createUniqueChild(n, 'dfar', 'DataArray_t', value=dfar)
    Internal.createUniqueChild(n, 'inv', 'DataArray_t', value=inv)
    # Set Extractions triggers in .Solver#define
    if VARS[4].get() == "1": value = 1
    else: value = 0
    Internal.createUniqueChild(n, 'extractWalls', 'DataArray_t', value=value)
    if VARS[5].get() == "1": value = 1
    else: value = 0
    Internal.createUniqueChild(n, 'extractLoads', 'DataArray_t', value=value)

    # remesh surface eventually
    dim = Internal.getZoneDim(z)
    remesh = False
    if dim[0] == 'Structured' and dim[2] == 1 and dim[3] == 1: remesh = True
    if dim[0] == 'Unstructured' and dim[3] == 'BAR': remesh = True
    if remesh: D._uniformize(z, h=float(snear))
    return None

#==============================================================================
# Set data in selected zones
#==============================================================================
def setData():
    if CTK.t == []: return
    
    snear = VARS[0].get()
    ibctype = VARS[1].get()
    dfar = VARS[2].get()
    if VARS[3].get() == 'out': inv = 0
    else: inv = 1
    
    nzs = CPlot.getSelectedZones()
    if nzs == []:
        CTK.TXT.insert('START', 'Selection is empty.\n')
        CTK.TXT.insert('START', 'Error: ', 'Error'); return
    
    CTK.saveTree()

    for nz in nzs:
        nob = CTK.Nb[nz]+1
        noz = CTK.Nz[nz]
        z = CTK.t[2][nob][2][noz]
        _setDataInZone(z, snear, ibctype, dfar, inv)
        CTK.replace(CTK.t, nob, noz, z)

    (CTK.Nb, CTK.Nz) = CPlot.updateCPlotNumbering(CTK.t)
    CTK.TKTREE.updateApp()
    CPlot.render()
    CTK.TXT.insert('START', 'IBC data set on surface.\n')

#==============================================================================
# Get data from selected zone
#==============================================================================
def getData():
    if CTK.t == []: return
    nzs = CPlot.getSelectedZones()
    if nzs == []:
        zone = Internal.getNodeFromType2(CTK.t, 'Zone_t')
    else: # get first of selection
        nz = nzs[0]
        nob = CTK.Nb[nz]+1
        noz = CTK.Nz[nz]
        zone = CTK.t[2][nob][2][noz]
    if zone is not None:
        n = Internal.getNodeFromPath(zone, '.Solver#define/snear')
        if n is not None:
            val = Internal.getValue(n)
            VARS[0].set(val)
        n = Internal.getNodeFromPath(zone, '.Solver#define/ibctype')
        if n is not None:
            val = Internal.getValue(n)
            VARS[1].set(val)
        n = Internal.getNodeFromPath(zone, '.Solver#define/dfar')
        if n is not None:
            val = Internal.getValue(n)
            VARS[2].set(val)
        n = Internal.getNodeFromPath(zone, '.Solver#define/inv')
        if n is not None:
            val = Internal.getValue(n)
            if val == 0: VARS[3].set('out')
            else: VARS[3].set('in')
        n = Internal.getNodeFromPath(zone, '.Solver#define/extractWalls')
        if n is not None:
            val = Internal.getValue(n)
            if val == 0: VARS[4].set('0')
            else: VARS[4].set('1')
        n = Internal.getNodeFromPath(zone, '.Solver#define/extractLoads')
        if n is not None:
            val = Internal.getValue(n)
            if val == 0: VARS[5].set('0')
            else: VARS[5].set('1')

#==============================================================================
# Create app widgets
#==============================================================================
def createApp(win):
    # - Frame -
    Frame = TTK.LabelFrame(win, borderwidth=2, relief=CTK.FRAMESTYLE,
                           text='tkIBC  [ + ]  ', font=CTK.FRAMEFONT, 
                           takefocus=1)
    Frame.bind('<Control-w>', hideApp)
    Frame.bind('<ButtonRelease-1>', displayFrameMenu)
    Frame.bind('<ButtonRelease-3>', displayFrameMenu)
    Frame.bind('<Enter>', lambda event : Frame.focus_set())
    Frame.columnconfigure(0, weight=0)
    Frame.columnconfigure(1, weight=1)
    Frame.columnconfigure(2, weight=0)
    WIDGETS['frame'] = Frame
    
    # - Frame menu -
    FrameMenu = TTK.Menu(Frame, tearoff=0)
    FrameMenu.add_command(label='Close', accelerator='Ctrl+w', command=hideApp)
    CTK.addPinMenu(FrameMenu, 'tkIBC')
    WIDGETS['frameMenu'] = FrameMenu

    # - VARS -
    # -0- Snear -
    V = TK.DoubleVar(win); V.set(0.01); VARS.append(V)
    # -1- IBC type -
    V = TK.StringVar(win); V.set('Musker'); VARS.append(V)
    # -2- dfar local -
    V = TK.DoubleVar(win); V.set(20.); VARS.append(V)
    # -3- mask inv or not -
    V = TK.StringVar(win); V.set('out'); VARS.append(V)
    # -4- extract fields on surface
    V = TK.StringVar(win); V.set('0'); VARS.append(V)
    # -5- extract loads for each component
    V = TK.StringVar(win); V.set('0'); VARS.append(V)
    
    # - Snear settings -
    B = TTK.Label(Frame, text="snear")
    B.grid(row=0, column=0, sticky=TK.EW)
    BB = CTK.infoBulle(parent=B, text='The generated grid spacing for selected curve.')
    B = TTK.Entry(Frame, textvariable=VARS[0], width=4, background="White")
    B.grid(row=0, column=1, columnspan=2, sticky=TK.EW)
    
    # - dfar settings  -
    B = TTK.Label(Frame, text="dfar")
    B.grid(row=1, column=0, sticky=TK.EW)
    BB = CTK.infoBulle(parent=B, text='The distance from the center of object to the far boundary.\nIf set to -1, not taken into account.')
    B = TTK.Entry(Frame, textvariable=VARS[2], width=4, background="White")
    B.grid(row=1, column=1, columnspan=2, sticky=TK.EW)

    # - IBC type -
    B = TTK.Label(Frame, text="IBC type")
    B.grid(row=2, column=0, sticky=TK.EW)
    BB = CTK.infoBulle(parent=B, text='Type of Immersed Boundary Condition.')
    B = TTK.OptionMenu(Frame, VARS[1], 'slip', 'noslip', 'Log', 'Musker', 'outpress', 'inj', 'TBLE', 'slip_cr', 'overlap')
    B.grid(row=2, column=1, columnspan=2, sticky=TK.EW)

    # - Mask settings (in or out) -
    B = TTK.Label(Frame, text="Fluid")
    B.grid(row=3, column=0, sticky=TK.EW)
    B = TTK.OptionMenu(Frame, VARS[3], 'out', 'in')
    B.grid(row=3, column=1, columnspan=2, sticky=TK.EW)

    # - Extract fields on surface
    B = TTK.Label(Frame, text="Extract")
    B.grid(row=4, column=0, sticky=TK.EW)
    B = TTK.Checkbutton(Frame, text='Wall Fields', variable=VARS[4])
    BB = CTK.infoBulle(parent=B, text='Extract various fields on surface.')
    B.grid(row=4, column=1, sticky=TK.EW)

    # - Extract loads on components
    B = TTK.Label(Frame, text="Extract")
    B.grid(row=5, column=0, sticky=TK.EW)
    B = TTK.Checkbutton(Frame, text='Loads', variable=VARS[5])
    BB = CTK.infoBulle(parent=B, text='Extract loads for each component.')
    B.grid(row=5, column=1, sticky=TK.EW)

    # - Set data -
    B = TTK.Button(Frame, text="Set data", command=setData)
    BB = CTK.infoBulle(parent=B, text='Set data into selected zone.')
    B.grid(row=6, column=0, columnspan=2, sticky=TK.EW)
    B = TTK.Button(Frame, text="Get data", command=getData,
                   image=iconics.PHOTO[8], padx=0, pady=0, compound=TK.RIGHT)
    BB = CTK.infoBulle(parent=B, text='Get data from selected zone.')
    B.grid(row=6, column=2, sticky=TK.EW)

#==============================================================================
# Called to display widgets
#==============================================================================
def showApp():
    #WIDGETS['frame'].grid(sticky=TK.NSEW)
    CTK.WIDGETS['BCNoteBook'].add(WIDGETS['frame'], text='tkIBC')
    CTK.WIDGETS['BCNoteBook'].select(WIDGETS['frame'])
    getData()

#==============================================================================
# Called to hide widgets
#==============================================================================
def hideApp(event=None):
    #WIDGETS['frame'].grid_forget()
    CTK.WIDGETS['BCNoteBook'].hide(WIDGETS['frame'])

#==============================================================================
# Update widgets when global pyTree t changes
#==============================================================================
def updateApp(): return

#==============================================================================
def displayFrameMenu(event=None):
    WIDGETS['frameMenu'].tk_popup(event.x_root+50, event.y_root, 0)

#==============================================================================
if __name__ == "__main__":
    import sys
    if len(sys.argv) == 2:
        CTK.FILE = sys.argv[1]
        try:
            CTK.t = C.convertFile2PyTree(CTK.FILE)
            (CTK.Nb, CTK.Nz) = CPlot.updateCPlotNumbering(CTK.t)
            CTK.display(CTK.t)
        except: pass

    # Main window
    (win, menu, file, tools) = CTK.minimal('tkIBC '+C.__version__)

    createApp(win); showApp()

    # - Main loop -
    win.mainloop()
