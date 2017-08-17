import FWCore.ParameterSet.Config as cms

process = cms.Process( "HLT" )

process.HLTConfigVersion = cms.PSet(
  tableName = cms.string('/users/mommsen/daqtest_ppemu/V1')
)

process.transferSystem = cms.PSet(
  destinations = cms.vstring( 'Tier0',
    'DQM',
    'ECAL',
    'EventDisplay',
    'Lustre',
    'None' ),
  transferModes = cms.vstring( 'default',
    'test',
    'emulator' ),
  streamA = cms.PSet(
    default = cms.vstring( 'Tier0' ),
    test = cms.vstring( 'Lustre' ),
    emulator = cms.vstring( 'Lustre' )
  ),
  streamDQM = cms.PSet(
    default = cms.vstring( 'DQM' ),
    test = cms.vstring( 'DQM',
      'Lustre' ),
    emulator = cms.vstring( 'None' )
  ),
  default = cms.PSet(
    default = cms.vstring( 'Tier0' ),
    test = cms.vstring( 'Lustre' ),
    emulator = cms.vstring( 'Lustre' )
  ),
  streamDQMHistograms = cms.PSet(
    default = cms.vstring( 'DQM' ),
    test = cms.vstring( 'DQM',
      'Lustre' ),
    emulator = cms.vstring( 'None' )
  ),
  streamHLTRates = cms.PSet(
    default = cms.vstring( 'Lustre' ),
    test = cms.vstring( 'Lustre' ),
    emulator = cms.vstring( 'None' )
  )
)
process.streams = cms.PSet(
  DQM = cms.vstring( 'DQM' ),
  ALCAELECTRON = cms.vstring( 'dALCAELECTRON' ),
  ALCALUMIPIXELS = cms.vstring( 'dALCALUMIPIXELS' ),
  ALCAP0 = cms.vstring( 'dALCAP0' ),
  ALCAPHISYM = cms.vstring( 'dALCAPHISYM' ),
  Calibration = cms.vstring( 'dCalibration' ),
  DQMCalibration = cms.vstring( 'dDQMCalibration' ),
  DQMEventDisplay = cms.vstring( 'dDQMEventDisplay' ),
  EcalCalibration = cms.vstring( 'dEcalCalibration' ),
  Express = cms.vstring( 'dExpress' ),
  HLTMonitor = cms.vstring( 'dHLTMonitor' ),
  NanoDST = cms.vstring( 'dNanoDST' ),
  Parking = cms.vstring( 'dParking' ),
  ParkingEGammaCommissioning = cms.vstring( 'dParkingEGammaCommissioning' ),
  PhysicsEndOfFill = cms.vstring( 'dPhysicsEndOfFill' ),
  PhysicsHadronsTaus = cms.vstring( 'dPhysicsHadronsTaus' ),
  PhysicsMuons = cms.vstring( 'dPhysicsMuons' ),
  PhysicsParkingScouting = cms.vstring( 'dPhysicsParkingScouting' ),
  PhysicsTest = cms.vstring( 'dPhysicsTest' ),
  RPCMON = cms.vstring( 'dRPCMON' ),
  ScoutingCalo = cms.vstring( 'dScoutingCalo' ),
  ScoutingPF = cms.vstring( 'dScoutingPF' )
)
process.datasets = cms.PSet(
  DQM = cms.vstring( 'pDQM' ),
  dALCAELECTRON = cms.vstring( 'pALCAELECTRON' ),
  dALCALUMIPIXELS = cms.vstring( 'pALCALUMIPIXELS' ),
  dALCAP0 = cms.vstring( 'pALCAP0' ),
  dALCAPHISYM = cms.vstring( 'pALCAPHISYM' ),
  dCalibration = cms.vstring( 'pCalibration' ),
  dDQMCalibration = cms.vstring( 'pDQMCalibration' ),
  dDQMEventDisplay = cms.vstring( 'pDQMEventDisplay' ),
  dEcalCalibration = cms.vstring( 'pEcalCalibration' ),
  dExpress = cms.vstring( 'pExpress' ),
  dHLTMonitor = cms.vstring( 'pHLTMonitor' ),
  dNanoDST = cms.vstring( 'pNanoDST' ),
  dParking = cms.vstring( 'pParking' ),
  dParkingEGammaCommissioning = cms.vstring( 'pParkingEGammaCommissioning' ),
  dPhysicsEndOfFill = cms.vstring( 'pPhysicsEndOfFill' ),
  dPhysicsHadronsTaus = cms.vstring( 'pPhysicsHadronsTaus' ),
  dPhysicsMuons = cms.vstring( 'pPhysicsMuons' ),
  dPhysicsParkingScouting = cms.vstring( 'pPhysicsParkingScouting' ),
  dPhysicsTest = cms.vstring( 'pPhysicsTest' ),
  dRPCMON = cms.vstring( 'pRPCMON' ),
  dScoutingCalo = cms.vstring( 'pScoutingCalo' ),
  dScoutingPF = cms.vstring( 'pScoutingPF' )
)

process.source = cms.Source( "FedRawDataInputSource",
    numBuffers = cms.untracked.uint32( 2 ),
    useL1EventID = cms.untracked.bool( True ),
    eventChunkSize = cms.untracked.uint32( 32 ),
    fileNames = cms.untracked.vstring(  ),
    verifyChecksum = cms.untracked.bool( True ),
    eventChunkBlock = cms.untracked.uint32( 32 ),
    verifyAdler32 = cms.untracked.bool( False ),
    maxBufferedFiles = cms.untracked.uint32( 2 ),
    fileListMode = cms.untracked.bool( False )
)

process.PoolDBESSource = cms.ESSource( "PoolDBESSource",
    globaltag = cms.string( "92X_dataRun2_HLT_v4" ),
    RefreshEachRun = cms.untracked.bool( True ),
    snapshotTime = cms.string( "" ),
    toGet = cms.VPSet(
    ),
    DBParameters = cms.PSet(
      authenticationPath = cms.untracked.string( "." ),
      connectionRetrialTimeOut = cms.untracked.int32( 60 ),
      idleConnectionCleanupPeriod = cms.untracked.int32( 10 ),
      messageLevel = cms.untracked.int32( 0 ),
      enablePoolAutomaticCleanUp = cms.untracked.bool( False ),
      enableConnectionSharing = cms.untracked.bool( True ),
      enableReadOnlySessionOnUpdateConnection = cms.untracked.bool( False ),
      connectionTimeOut = cms.untracked.int32( 0 ),
      authenticationSystem = cms.untracked.int32( 0 ),
      connectionRetrialPeriod = cms.untracked.int32( 10 )
    ),
    RefreshAlways = cms.untracked.bool( False ),
    connect = cms.string( "frontier://FrontierProd/CMS_CONDITIONS" ),
    ReconnectEachRun = cms.untracked.bool( False ),
    RefreshOpenIOVs = cms.untracked.bool( False ),
    DumpStat = cms.untracked.bool( False )
)

process.DQMStore = cms.Service( "DQMStore",
    verbose = cms.untracked.int32( 0 ),
    collateHistograms = cms.untracked.bool( False ),
    enableMultiThread = cms.untracked.bool( True ),
    forceResetOnBeginLumi = cms.untracked.bool( False ),
    LSbasedMode = cms.untracked.bool( True ),
    verboseQT = cms.untracked.int32( 0 )
)
process.EvFDaqDirector = cms.Service( "EvFDaqDirector",
    buBaseDir = cms.untracked.string( "." ),
    baseDir = cms.untracked.string( "." ),
    fuLockPollInterval = cms.untracked.uint32( 2000 ),
    runNumber = cms.untracked.uint32( 0 ),
    microMergeDisabled = cms.untracked.bool( False ),
    outputAdler32Recheck = cms.untracked.bool( False ),
    selectedTransferMode = cms.untracked.string( "" ),
    requireTransfersPSet = cms.untracked.bool( False ),
    emptyLumisectionMode = cms.untracked.bool( False )
)
process.FastMonitoringService = cms.Service( "FastMonitoringService",
    fastMonIntervals = cms.untracked.uint32( 2 ),
    sleepTime = cms.untracked.int32( 1 )
)
process.PrescaleService = cms.Service( "PrescaleService",
    forceDefault = cms.bool( True ),
    prescaleTable = cms.VPSet(
      cms.PSet(  pathName = cms.string( "pDQM" ),
        prescales = cms.vuint32( 0, 423, 0, 846 )
      ),
      cms.PSet(  pathName = cms.string( "pALCALUMIPIXELS" ),
        prescales = cms.vuint32( 372, 372, 743, 743 )
      ),
      cms.PSet(  pathName = cms.string( "pParking" ),
        prescales = cms.vuint32( 105, 105, 211, 211 )
      ),
      cms.PSet(  pathName = cms.string( "pNanoDST" ),
        prescales = cms.vuint32( 1245, 1245, 2491, 2491 )
      ),
      cms.PSet(  pathName = cms.string( "pHLTMonitor" ),
        prescales = cms.vuint32( 5801, 5801, 11603, 11603 )
      ),
      cms.PSet(  pathName = cms.string( "pExpress" ),
        prescales = cms.vuint32( 905, 905, 1810, 1810 )
      ),
      cms.PSet(  pathName = cms.string( "pCalibration" ),
        prescales = cms.vuint32( 10434, 10434, 20868, 20868 )
      ),
      cms.PSet(  pathName = cms.string( "pALCAPHISYM" ),
        prescales = cms.vuint32( 6318, 6318, 12637, 12637 )
      ),
      cms.PSet(  pathName = cms.string( "pALCAP0" ),
        prescales = cms.vuint32( 607, 607, 1213, 1213 )
      ),
      cms.PSet(  pathName = cms.string( "pALCAELECTRON" ),
        prescales = cms.vuint32( 1345, 1345, 2691, 2691 )
      ),
      cms.PSet(  pathName = cms.string( "pPhysicsEndOfFill" ),
        prescales = cms.vuint32( 0, 0, 0, 0 )
      ),
      cms.PSet(  pathName = cms.string( "pPhysicsHadronsTaus" ),
        prescales = cms.vuint32( 135, 135, 270, 270 )
      ),
      cms.PSet(  pathName = cms.string( "pPhysicsMuons" ),
        prescales = cms.vuint32( 164, 164, 329, 329 )
      ),
      cms.PSet(  pathName = cms.string( "pPhysicsParkingScouting" ),
        prescales = cms.vuint32( 704, 704, 1409, 1409 )
      ),
      cms.PSet(  pathName = cms.string( "pRPCMON" ),
        prescales = cms.vuint32( 882, 882, 1764, 1764 )
      ),
      cms.PSet(  pathName = cms.string( "pScoutingCalo" ),
        prescales = cms.vuint32( 5347, 5347, 10693, 10693 )
      ),
      cms.PSet(  pathName = cms.string( "pScoutingPF" ),
        prescales = cms.vuint32( 4954, 4954, 9908, 9908 )
      ),
      cms.PSet(  pathName = cms.string( "pPhysicsTest" ),
        prescales = cms.vuint32( 0, 0, 0, 0 )
      ),
      cms.PSet(  pathName = cms.string( "pParkingEGammaCommissioning" ),
        prescales = cms.vuint32( 88, 88, 176, 176 )
      ),
      cms.PSet(  pathName = cms.string( "pDQMEventDisplay" ),
        prescales = cms.vuint32( 0, 3677, 0, 7353 )
      ),
      cms.PSet(  pathName = cms.string( "pEcalCalibration" ),
        prescales = cms.vuint32( 15672, 15672, 31344, 31344 )
      ),
      cms.PSet(  pathName = cms.string( "pDQMCalibration" ),
        prescales = cms.vuint32( 0, 10434, 0, 20868 )
      )
    ),
    lvl1DefaultLabel = cms.string( "4GBs" ),
    lvl1Labels = cms.vstring(
      '4GBs_noDQM',
      '4GBs',
      '2GBs_noDQM',
      '2GBs')
)
process.MessageLogger = cms.Service( "MessageLogger",
    suppressInfo = cms.untracked.vstring( 'hltGtDigis' ),
    debugs = cms.untracked.PSet(
      threshold = cms.untracked.string( "INFO" ),
      placeholder = cms.untracked.bool( True ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    suppressDebug = cms.untracked.vstring(  ),
    cout = cms.untracked.PSet(
      threshold = cms.untracked.string( "ERROR" ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    cerr_stats = cms.untracked.PSet(
      threshold = cms.untracked.string( "WARNING" ),
      output = cms.untracked.string( "cerr" ),
      optionalPSet = cms.untracked.bool( True )
    ),
    warnings = cms.untracked.PSet(
      threshold = cms.untracked.string( "INFO" ),
      placeholder = cms.untracked.bool( True ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    statistics = cms.untracked.vstring( 'cerr' ),
    cerr = cms.untracked.PSet(
      INFO = cms.untracked.PSet(  limit = cms.untracked.int32( 0 ) ),
      noTimeStamps = cms.untracked.bool( False ),
      FwkReport = cms.untracked.PSet(
        reportEvery = cms.untracked.int32( 1 ),
        limit = cms.untracked.int32( 0 )
      ),
      default = cms.untracked.PSet(  limit = cms.untracked.int32( 10000000 ) ),
      Root_NoDictionary = cms.untracked.PSet(  limit = cms.untracked.int32( 0 ) ),
      FwkJob = cms.untracked.PSet(  limit = cms.untracked.int32( 0 ) ),
      FwkSummary = cms.untracked.PSet(
        reportEvery = cms.untracked.int32( 1 ),
        limit = cms.untracked.int32( 10000000 )
      ),
      threshold = cms.untracked.string( "INFO" ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    FrameworkJobReport = cms.untracked.PSet(
      default = cms.untracked.PSet(  limit = cms.untracked.int32( 0 ) ),
      FwkJob = cms.untracked.PSet(  limit = cms.untracked.int32( 10000000 ) )
    ),
    suppressWarning = cms.untracked.vstring( 'hltGtDigis' ),
    errors = cms.untracked.PSet(
      threshold = cms.untracked.string( "INFO" ),
      placeholder = cms.untracked.bool( True ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    fwkJobReports = cms.untracked.vstring( 'FrameworkJobReport' ),
    debugModules = cms.untracked.vstring(  ),
    infos = cms.untracked.PSet(
      threshold = cms.untracked.string( "INFO" ),
      Root_NoDictionary = cms.untracked.PSet(  limit = cms.untracked.int32( 0 ) ),
      placeholder = cms.untracked.bool( True ),
      suppressInfo = cms.untracked.vstring(  ),
      suppressWarning = cms.untracked.vstring(  ),
      suppressDebug = cms.untracked.vstring(  ),
      suppressError = cms.untracked.vstring(  )
    ),
    categories = cms.untracked.vstring( 'FwkJob',
      'FwkReport',
      'FwkSummary',
      'Root_NoDictionary' ),
    destinations = cms.untracked.vstring( 'warnings',
      'errors',
      'infos',
      'debugs',
      'cout',
      'cerr' ),
    threshold = cms.untracked.string( "INFO" ),
    suppressError = cms.untracked.vstring( 'hltGtDigis' )
)

process.DQMFileSaver = cms.EDAnalyzer( "DQMFileSaver",
    runIsComplete = cms.untracked.bool( False ),
    referenceHandling = cms.untracked.string( "all" ),
    producer = cms.untracked.string( "DQM" ),
    forceRunNumber = cms.untracked.int32( -1 ),
    saveByRun = cms.untracked.int32( 1 ),
    saveAtJobEnd = cms.untracked.bool( False ),
    saveByLumiSection = cms.untracked.int32( 1 ),
    version = cms.untracked.int32( 1 ),
    referenceRequireStatus = cms.untracked.int32( 100 ),
    convention = cms.untracked.string( "FilterUnit" ),
    dirName = cms.untracked.string( "." ),
    fileFormat = cms.untracked.string( "PB" )
)

process.hltHLTriggerJSONMonitoring = cms.EDAnalyzer( "HLTriggerJSONMonitoring",
    triggerResults = cms.InputTag( 'TriggerResults','','HLT' )
)

process.hltGtDigis = cms.EDProducer( "L1GlobalTriggerRawToDigi",
    DaqGtFedId = cms.untracked.int32( 813 ),
    Verbosity = cms.untracked.int32( 0 ),
    UnpackBxInEvent = cms.int32( 5 ),
    ActiveBoardsMask = cms.uint32( 0xffff ),
    DaqGtInputTag = cms.InputTag( "rawDataCollector" )
)
process.hltL1GtObjectMap = cms.EDProducer( "L1GlobalTrigger",
    TechnicalTriggersUnprescaled = cms.bool( True ),
    ProduceL1GtObjectMapRecord = cms.bool( True ),
    AlgorithmTriggersUnmasked = cms.bool( False ),
    EmulateBxInEvent = cms.int32( 1 ),
    AlgorithmTriggersUnprescaled = cms.bool( True ),
    ProduceL1GtDaqRecord = cms.bool( False ),
    ReadTechnicalTriggerRecords = cms.bool( True ),
    RecordLength = cms.vint32( 3, 0 ),
    TechnicalTriggersUnmasked = cms.bool( False ),
    ProduceL1GtEvmRecord = cms.bool( False ),
    GmtInputTag = cms.InputTag( "hltGtDigis" ),
    TechnicalTriggersVetoUnmasked = cms.bool( True ),
    AlternativeNrBxBoardEvm = cms.uint32( 0 ),
    TechnicalTriggersInputTags = cms.VInputTag( 'simBscDigis' ),
    CastorInputTag = cms.InputTag( "castorL1Digis" ),
    GctInputTag = cms.InputTag( "hltGctDigis" ),
    AlternativeNrBxBoardDaq = cms.uint32( 0 ),
    WritePsbL1GtDaqRecord = cms.bool( False ),
    BstLengthBytes = cms.int32( -1 )
)
process.ExceptionGenerator2 = cms.EDAnalyzer( "ExceptionGenerator",
    defaultAction = cms.untracked.int32(0),
    defaultQualifier = cms.untracked.int32(0)#,
    #defaultAction = cms.untracked.int32(15),
    #defaultQualifier = cms.untracked.int32(45)#,
    #secondQualifier = cms.untracked.double(5),
    #thirdQualifier = cms.untracked.double(0.5)
)
process.hltPrepDQM = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepALCALUMIPIXELS = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepParking = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepNanoDST = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepHLTMonitor = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepExpress = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepCalibration = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepALCAPHISYM = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepALCAP0 = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepALCAELECTRON = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepPhysicsEndOfFill = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepPhysicsHadronsTaus = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepPhysicsMuons = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepPhysicsParkingScouting = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepRPCMON = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepScoutingCalo = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepScoutingPF = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepPhysicsTest = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepParkingEGammaCommissioning = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.ExceptionGenerator1 = cms.EDAnalyzer( "ExceptionGenerator",
    defaultAction = cms.untracked.int32( 0 ),
    defaultQualifier = cms.untracked.int32( 0 )
)
process.hltPrepDQMEventDisplay = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepEcalCalibration = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)
process.hltPrepDQMCalibration = cms.EDFilter( "HLTPrescaler",
    L1GtReadoutRecordTag = cms.InputTag( "hltGtDigis" ),
    offset = cms.uint32( 0 )
)

process.hltOutputParkingEGammaCommissioning = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pParkingEGammaCommissioning' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputPhysicsTest = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pPhysicsTest' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputScoutingPF = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pScoutingPF' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputScoutingCalo = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pScoutingCalo' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputRPCMON = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pRPCMON' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputPhysicsParkingScouting = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pPhysicsParkingScouting' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputPhysicsMuons = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pPhysicsMuons' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputPhysicsHadronsTaus = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pPhysicsHadronsTaus' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputPhysicsEndOfFill = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pPhysicsEndOfFill' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputParking = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pParking' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputALCALUMIPIXELS = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pALCALUMIPIXELS' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputNanoDST = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pNanoDST' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputHLTMonitor = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pHLTMonitor' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputExpress = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pExpress' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputCalibration = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pCalibration' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputALCAPHISYM = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pALCAPHISYM' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputALCAP0 = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pALCAP0' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputALCAELECTRON = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pALCAELECTRON' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputDQM = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pDQM' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputDQMEventDisplay = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pDQMEventDisplay' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputEcalCalibration = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pEcalCalibration' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)
process.hltOutputDQMCalibration = cms.OutputModule( "ShmStreamConsumer",
    SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring( 'pDQMCalibration' ) ),
    outputCommands = cms.untracked.vstring( 'drop *',
      'keep FEDRawDataCollection_rawDataCollector_*_*',
      'keep FEDRawDataCollection_source_*_*' )
)

process.epParkingEGammaCommissioning = cms.EndPath( process.hltOutputParkingEGammaCommissioning )
process.epPhysicsTest = cms.EndPath( process.hltOutputPhysicsTest )
process.epScoutingPF = cms.EndPath( process.hltOutputScoutingPF )
process.epScoutingCalo = cms.EndPath( process.hltOutputScoutingCalo )
process.epRPCMON = cms.EndPath( process.hltOutputRPCMON )
process.epParkingScouting = cms.EndPath( process.hltOutputPhysicsParkingScouting )
process.epPhysicsMuons = cms.EndPath( process.hltOutputPhysicsMuons )
process.epPhysicsHadronsTaus = cms.EndPath( process.hltOutputPhysicsHadronsTaus )
process.epPhysicsEndOfFill = cms.EndPath( process.hltOutputPhysicsEndOfFill )
process.epParking = cms.EndPath( process.hltOutputParking )
process.epALCALUMIPIXELS = cms.EndPath( process.hltOutputALCALUMIPIXELS )
process.epNanoDST = cms.EndPath( process.hltOutputNanoDST )
process.epHLTMonitor = cms.EndPath( process.hltOutputHLTMonitor )
process.epExpress = cms.EndPath( process.hltOutputExpress )
process.epCalibration = cms.EndPath( process.hltOutputCalibration )
process.epALCAPHISYM = cms.EndPath( process.hltOutputALCAPHISYM )
process.epALCAP0 = cms.EndPath( process.hltOutputALCAP0 )
process.epALCAELECTRON = cms.EndPath( process.hltOutputALCAELECTRON )
process.pDQMhisto = cms.Path( process.DQMFileSaver )
process.json = cms.EndPath( process.hltHLTriggerJSONMonitoring )
process.L1Gt = cms.Path( process.hltGtDigis + process.hltL1GtObjectMap )
process.epDQM = cms.EndPath( process.hltOutputDQM )
process.epDQMEventDisplay = cms.EndPath( process.hltOutputDQMEventDisplay )
process.epEcalCalibration = cms.EndPath( process.hltOutputEcalCalibration )
process.epDQMCalibration = cms.EndPath( process.hltOutputDQMCalibration )
process.pDQM = cms.Path( process.ExceptionGenerator2 + process.hltPrepDQM )
process.pALCALUMIPIXELS = cms.Path( process.ExceptionGenerator2 + process.hltPrepALCALUMIPIXELS )
process.pParking = cms.Path( process.ExceptionGenerator2 + process.hltPrepParking )
process.pNanoDST = cms.Path( process.ExceptionGenerator2 + process.hltPrepNanoDST )
process.pHLTMonitor = cms.Path( process.ExceptionGenerator2 + process.hltPrepHLTMonitor )
process.pExpress = cms.Path( process.ExceptionGenerator2 + process.hltPrepExpress )
process.pCalibration = cms.Path( process.ExceptionGenerator2 + process.hltPrepCalibration )
process.pALCAPHISYM = cms.Path( process.ExceptionGenerator2 + process.hltPrepALCAPHISYM )
process.pALCAP0 = cms.Path( process.ExceptionGenerator2 + process.hltPrepALCAP0 )
process.pALCAELECTRON = cms.Path( process.ExceptionGenerator2 + process.hltPrepALCAELECTRON )
process.pPhysicsEndOfFill = cms.Path( process.ExceptionGenerator2 + process.hltPrepPhysicsEndOfFill )
process.pPhysicsHadronsTaus = cms.Path( process.ExceptionGenerator2 + process.hltPrepPhysicsHadronsTaus )
process.pPhysicsMuons = cms.Path( process.ExceptionGenerator2 + process.hltPrepPhysicsMuons )
process.pPhysicsParkingScouting = cms.Path( process.ExceptionGenerator2 + process.hltPrepPhysicsParkingScouting )
process.pRPCMON = cms.Path( process.ExceptionGenerator2 + process.hltPrepRPCMON )
process.pScoutingCalo = cms.Path( process.ExceptionGenerator2 + process.hltPrepScoutingCalo )
process.pScoutingPF = cms.Path( process.ExceptionGenerator2 + process.hltPrepScoutingPF )
process.pPhysicsTest = cms.Path( process.ExceptionGenerator2 + process.hltPrepPhysicsTest )
process.pParkingEGammaCommissioning = cms.Path( process.ExceptionGenerator2 + process.hltPrepParkingEGammaCommissioning )
process.pDQMEventDisplay = cms.Path( process.ExceptionGenerator1 + process.hltPrepDQMEventDisplay )
process.pEcalCalibration = cms.Path( process.ExceptionGenerator1 + process.hltPrepEcalCalibration )
process.pDQMCalibration = cms.Path( process.ExceptionGenerator1 + process.hltPrepDQMCalibration )

import FWCore.ParameterSet.VarParsing as VarParsing

import os

cmsswbase = os.path.expandvars('$CMSSW_BASE/')

options = VarParsing.VarParsing ('analysis')

options.register ('runNumber',
                  1, # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.int,          # string, int, or float
                  "Run Number")

options.register ('buBaseDir',
                  '/fff/BU0', # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,          # string, int, or float
                  "BU base directory")

options.register ('dataDir',
                  '/fff/data', # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,          # string, int, or float
                  "FU data directory")

options.register ('numThreads',
                  1, # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.int,          # string, int, or float
                  "Number of CMSSW threads")

options.register ('numFwkStreams',
                  1, # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.int,          # string, int, or float
                  "Number of CMSSW streams")

options.register ('transferMode',
                  '', # default value
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,          # string, int, or float
                  "Selected transfer mode propagated by RCMS")

options.parseArguments()

process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(options.numThreads),
    numberOfStreams = cms.untracked.uint32(options.numFwkStreams),
    multiProcesses = cms.untracked.PSet(
    maxChildProcesses = cms.untracked.int32(0)
    )
)

process.EvFDaqDirector.buBaseDir    = options.buBaseDir
process.EvFDaqDirector.baseDir      = options.dataDir
process.EvFDaqDirector.runNumber    = options.runNumber

try:
     process.EvFDaqDirector.selectedTransferMode = options.transferMode
except:
     print "unable to set process.EvFDaqDirector.selectedTransferMode=",         options.transferMode
for moduleName in process.__dict__['_Process__outputmodules']:
    modified_module = getattr(process,moduleName)
    modified_module.compression_level=cms.untracked.int32(0)
