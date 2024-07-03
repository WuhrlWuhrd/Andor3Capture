from threading import Thread
from time import sleep
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5 import uic
import re
import os

path = os.path.dirname(__file__)

class ZylaGUI(QWidget):
    
    features = "AccumulateCount, Integer; AcquisitionStart, Command; AcquisitionStop, Command; AlternatingReadoutDirection, Boolean; AOIBinning, Enumerated; AOIHBin, Integer; AOIHeight, Integer; AOILeft, Integer; AOIStride, Integer; AOITop, Integer; AOIVBin, Integer; AOIWidth, Integer; AuxiliaryOutSource, Enumerated; AuxOutSourceTwo, Enumerated; Baseline, Integer; BitDepth, Enumerated; BufferOverflowEvent, Integer; BytesPerPixel, Float; CameraAcquiring, Boolean; CameraModel, String; CameraName, String; CameraPresent, Boolean; ControllerID, String; CoolerPower, Float; CycleMode, Enumerated; DeviceCount, Integer; DeviceVideoIndex, Integer; ElectronicShutteringMode, Enumerated; EventEnable, Boolean; EventSelector, Enumerated; EventsMissedEvent, Integer; ExposedPixelHeight, Integer; ExposureEndEvent, Integer; ExposureStartEvent, Integer; ExposureTime, Float; ExternalTriggerDelay, Float; FanSpeed, Enumerated; FastAOIFrameRateEnable, Boolean; FirmwareVersion, String; FrameCount, Integer; FrameRate, Float; FullAOIControl, Boolean; ImageSizeBytes, Integer; InterfaceType, String; IOInvert, Boolean; IOSelector, Enumerated; LineScanSpeed, Float; LogLevel, Enumerated; LongExposureTransition, Float; MaxInterfaceTransferRate, Float; MetadataEnable, Boolean; MetadataFrame, Boolean; MetadataTimestamp, Boolean; MultitrackBinned, Boolean; MultitrackCount, Integer; MultitrackEnd, Integer; MultitrackSelector, Integer; MultitrackStart, Integer; Overlap, Boolean; PixelEncoding, Enumerated; PixelHeight, Float; PixelReadoutRate, Enumerated; PixelWidth, Float; ReadoutTime, Float; RollingShutterGlobalClear, Boolean; RowNExposureEndEvent, Integer; RowNExposureStartEvent, Integer; RowReadTime, FloatingPoint; ScanSpeedControlEnable, Boolean; SensorCooling, Boolean; SensorHeight, Integer; SensorReadoutMode, Enumerated; SensorTemperature, Float; SensorWidth, Integer; SerialNumber, String; ShutterMode, Enumerated; ShutterOutputMode, Enumerated; ShutterTransferTime, Float; SimplePreAmpGainControl, Enumerated; SoftwareTrigger, Command; SoftwareVersion, String; SpuriousNoiseFilter, Boolean; StaticBlemishCorrection, Boolean; TemperatureControl, Enumerated; TemperatureStatus, Enumerated; TimestampClock, Integer; TimestampClockFrequency, Integer; TimestampClockReset, Command; TriggerMode, Enumerated; VerticallyCentreAOI, Boolean"
    
    def __init__(self, camera = None) -> None:
        
        super().__init__()
        
        self._camera = camera
        
        uic.loadUi(f"{path}/andor3.ui", self)
        
        self._numberOfFrames     = self.findChild(QSpinBox, "numberOfFrames")
        self._exposureTime       = self.findChild(QDoubleSpinBox, "exposureTime")
        self._div5Button         = self.findChild(QPushButton, "dev5Button")
        self._mult5Button        = self.findChild(QPushButton, "mult5Button")
        self._captureButton      = self.findChild(QPushButton, "captureButton")
        self._abortButton        = self.findChild(QPushButton, "aboutButton")
        self._takeBGButton       = self.findChild(QPushButton, "takeBGButton")
        self._removeBGButton     = self.findChild(QPushButton, "removeBGButton")
        self._streamToDiskButton = self.findChild(QPushButton, "streamToDiskButton")
        self._aoiLayout          = self.findChild(QComboBox, "aoiLayout")
        self._editTracksButton   = self.findChild(QPushButton, "editTracksButton")
        self._refreshButton      = self.findChild(QPushButton, "refreshButton")
        self._applyButton        = self.findChild(QPushButton, "applyButton")
        self._cameraParameters   = self.findChild(QFormLayout, "cameraParameters")
        
        self.populateParameters()
        
        self._aoiLayout.currentIndexChanged.connect(self.updateTracksButton)
        
        
        
        
    def populateParameters(self):
        
        rows = ZylaGUI.features.split("; ")
        
        for row in rows:
            
            (name, type) = row.split(", ")
            
            if type == "Integer":
                field = QSpinBox()
            elif type == "Float":
                field = QDoubleSpinBox()
            elif type == "Boolean":
                field = QComboBox()
                field.addItem("True", True)
                field.addItem("False", False)
            elif type == "Enumerated":
                field = QComboBox()
                if self._camera is not None:
                    options = getattr(self._camera, f"get{name}Options")()
                    for (key, value) in options.items():
                        field.addItem(value, key)
            else:
                continue
            
            label  = re.sub(r"([a-z])([A-Z])", r"\1 \2", name)
            label  = re.sub(r"([A-Z])([A-Z][a-z])", r"\1 \2", label)
            policy = field.sizePolicy()
            policy.setHorizontalPolicy(QSizePolicy.Policy.Expanding)
            field.setSizePolicy(policy)
            
            self._cameraParameters.addRow(label, field)
            
    
    def updateTracksButton(self):
        self._editTracksButton.setEnabled(self._aoiLayout.currentText() == "Multi-Track")
                

class ZylaStreamGUI(QWidget):
    
    def __init__(self, capture) -> None:
        
        super().__init__()
        
        self._updateTimer = QTimer(self)
        self._capture     = capture
        
        # Compile GUI from .ui file
        uic.loadUi(f"{path}/dataStream.ui", self)
        
        # Get handles on all important GUI elements
        self._outputFile               = self.findChild(QLineEdit, "outputFile")
        self._outputFileBrowse         = self.findChild(QPushButton, "outputFileBrowse")
        self._useH5Conversion          = self.findChild(QCheckBox, "useH5Conversion")
        self._h5ConversionOutput       = self.findChild(QLineEdit, "h5ConversionOutput")
        self._h5ConversionOutputBrowse = self.findChild(QPushButton, "h5ConversionOutputBrowse")
        self._frameLimit               = self.findChild(QSpinBox, "frameLimit")
        self._useFrameLimit            = self.findChild(QCheckBox, "useFrameLimit")
        self._startButton              = self.findChild(QPushButton, "startButton")
        self._stopButton               = self.findChild(QPushButton, "stopButton")
        self._stoppedLabel             = self.findChild(QLabel, "stoppedLabel")
        self._acquiringLabel           = self.findChild(QLabel, "acquiringLabel")
        self._processingLabel          = self.findChild(QLabel, "processingLabel")
        self._writingLabel             = self.findChild(QLabel, "writingLabel")
        self._convertingLabel          = self.findChild(QLabel, "convertingLabel")
        self._acquisitionRate          = self.findChild(QProgressBar, "acquisitionRate")
        self._processingRate           = self.findChild(QProgressBar, "processingRate")
        self._writingRate              = self.findChild(QProgressBar, "writingRate")
        self._acquisitionRateLabel     = self.findChild(QLabel, "acquisitionRateLabel")
        self._processingRateLabel      = self.findChild(QLabel, "processingRateLabel")
        self._writingRateLabel         = self.findChild(QLabel, "writingRateLabel")
        self._processingQueue          = self.findChild(QProgressBar, "processingQueue")
        self._writingQueue             = self.findChild(QProgressBar, "writingQueue")
        self._processingQueueLabel     = self.findChild(QLabel, "processingQueueLabel")
        self._writingQueueLabel        = self.findChild(QLabel, "writingQueueLabel")
        
        # Connect GUI events to callback functions
        self._useFrameLimit.stateChanged.connect(self.updateTicks)
        self._useH5Conversion.stateChanged.connect(self.updateTicks)
        self._updateTimer.timeout.connect(self.updateStatus)
        self._startButton.clicked.connect(self.start)
        self._stopButton.clicked.connect(self.stop)
        self._outputFileBrowse.clicked.connect(lambda: self.browse(self._outputFile))
        self._h5ConversionOutputBrowse.clicked.connect(lambda: self.browse(self._h5ConversionOutput))
        
        self.updateTicks()
        
        
    def updateTicks(self):
        
        self._frameLimit.setEnabled(self._useFrameLimit.isChecked())
        self._h5ConversionOutput.setEnabled(self._useH5Conversion.isChecked())
        self._h5ConversionOutputBrowse.setEnabled(self._useH5Conversion.isChecked())
        
    
    def setInterfaceEnabled(self, enabled: bool):
        
        self._outputFile.setEnabled(enabled)
        self._outputFileBrowse.setEnabled(enabled)
        self._useH5Conversion.setEnabled(enabled)
        self._h5ConversionOutput.setEnabled(enabled)
        self._h5ConversionOutputBrowse.setEnabled(enabled)
        self._frameLimit.setEnabled(enabled)
        self._useFrameLimit.setEnabled(enabled)
        
        if enabled:
            self.updateTicks()
        
        
    def start(self):
        
        try:
        
            self._startButton.setEnabled(False)
            self._stopButton.setEnabled(True)
            self.setInterfaceEnabled(False)
        
            if self._useFrameLimit.isChecked():
                self._capture.setFrameLimit(self._frameLimit.value())
            else:
                self._capture.setFrameLimit(-1)
                
            self._capture.setOutputPath(self._outputFile.text())
            self._capture.setVerbose(False)
            
            self._capture.start()
            
            self._updateTimer.start(500)
            
            
        except Exception as e:
            
            QMessageBox.critical(self, "Error", "Error starting acquisition:\n\n" + str(e))
            self.stop(False)
        
        
    def stop(self, errors=True):
        
        self._stopButton.setEnabled(False)
        
        try:
        
            self._capture.stop()
            
            while self._capture.isMonitoring():
                sleep(0.1)
                
            if self._useH5Conversion.isChecked():
                self.convert(self._outputFile.text(), self._h5ConversionOutput.text(), 2048, 8)
                
                
        except Exception as e:
            
            if errors:
                QMessageBox.critical(self, "Error", "Error stopping acquisition:\n\n" + str(e))
            
                
        finally:
                
            self._startButton.setEnabled(True)
            self._updateTimer.stop()
            self.setInterfaceEnabled(True)
        
        
    def browse(self, pathField: QLineEdit):
        
        file = QFileDialog.getSaveFileUrl(parent=self, options=QFileDialog.DontUseNativeDialog)
        
        if not file[0].isEmpty():
            pathField.setText(file[0].path())
        
            
    def convert(self, input: str, output: str, rowSize: int, noRows: int):
        
        self._convertingLabel.setStyleSheet("background: teal; color: white;")
        self._convertingLabel.setStyleSheet("background: silver; color: white;")
        
        
    def updateStatus(self):
        
        try:
        
            aFPS   = self._capture.getAcquireFPS()
            pFPS   = self._capture.getProcessFPS()
            wFPS   = self._capture.getWriteFPS()
            pQueue = self._capture.getProcessQueueSize()
            wQueue = self._capture.getWriteQueueSize()
            total  = self._capture.getAcquireCount()
            
            divisor = min([1, max([aFPS, pFPS, wFPS])])
            
            self._acquisitionRate.setValue(int(100 * aFPS / divisor))
            self._acquisitionRateLabel.setText("%d Hz" % int(aFPS))
            self._processingRate.setValue(int(100 * pFPS / divisor))
            self._processingRateLabel.setText("%d Hz" % int(pFPS))
            self._writingRate.setValue(int(100 * wFPS / divisor))
            self._writingRateLabel.setText("%d Hz" % int(wFPS))
            
            self._processingQueue.setValue(int(100 * pQueue / total))
            self._processingQueueLabel.setText("%d" % pQueue)
            self._writingQueue.setValue(int(100 * wQueue / total))
            self._writingQueueLabel.setText("%d" % wQueue)
            
            if self._capture.isRunning():
                
                self._stoppedLabel.setStyle("background: silver; color: white;")
            
                if aFPS > 0:
                    self._acquiringLabel.setStyleSheet("background: teal; color: white;")
                else:
                    self._acquiringLabel.setStyleSheet("background: brown; color: white;")
                    
                    
                if pFPS >= aFPS * 0.9:
                    self._processingLabel.setStyleSheet("background: teal; color: white;")
                elif pFPS > 0:
                    self._processingLabel.setStyleSheet("background: orange; color: white;")
                else:
                    self._processingLabel.setStyleSheet("background: brown; color: white;")
                    
                    
                if wFPS >= pFPS * 0.9:
                    self._writingLabel.setStyleSheet("background: teal; color: white;")
                elif wFPS > 0:
                    self._writingLabel.setStyleSheet("background: orange; color: white;")
                else:
                    self._writingLabel.setStyleSheet("background: brown; color: white;")          
            
            
            elif not self._capture.isMonitoring():
                
                self._acquiringLabel.setStyleSheet("background: silver; color: white;")
                self._processingLabel.setStyleSheet("background: silver; color: white;")
                self._writingLabel.setStyleSheet("background: silver; color: white;")
                self._stoppedLabel.setStyleSheet("background: brown; color: white;")
                    
                    
            else:
                
                self._stoppedLabel.setStyleSheet("background: orange; color: white;")
            
                if aFPS > 0:
                    self._acquiringLabel.setStyleSheet("background: teal; color: white;")
                else:
                    self._acquiringLabel.setStyleSheet("background: silver; color: white;")
                    
                    
                if pFPS > 0:
                    self._processingLabel.setStyleSheet("background: teal; color: white;")
                else:
                    self._processingLabel.setStyleSheet("background: silver; color: white;")
                    
                    
                if wFPS > 0:
                    self._writingLabel.setStyleSheet("background: teal; color: white;")
                else:
                    self._writingLabel.setStyleSheet("background: silver; color: white;")

                    
        except Exception as e:
                
                self._acquiringLabel.setStyleSheet("background: brown; color: white;")
                self._processingLabel.setStyleSheet("background: brown; color: white;")
                self._writingLabel.setStyleSheet("background: brown; color: white;")
                self._stoppedLabel.setStyleSheet("background: brown; color: white;")
                
            
        
        
app = QApplication([])
win = ZylaGUI()

win.show()
app.exec_()