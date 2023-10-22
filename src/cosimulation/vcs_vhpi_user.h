/*
 * vhpi_user.h
 *
 * The VHDL procedural interface draft standard header file. The contents
 * of this file are bound to change as the standard evolves and goes to
 * ballot. We will try to keep backward compatibility as far as possible.
 *
 */

#ifndef VHPI_H
#define VHPI_H
#ifndef VPD_PLI_VHPI_USER_HH
#ifdef __cplusplus
extern "C" {
#endif

/* VHPI Scalar Types */
typedef unsigned vhpiEnumT;
typedef int vhpiIntT;
typedef double vhpiRealT;

#ifdef Synopsys_Win32
typedef __int64 vhpiPhysT, vhpiTimeT;
#else
typedef long long vhpiPhysT, vhpiTimeT;
#endif

/* VHPI handle typedef */
typedef unsigned long* vhpiHandleT;

/* VHPI class kinds */
typedef enum {
    vhpiAccessTypeDeclK = 1001,
    vhpiAggregateK,
    vhpiAliasDeclK,
    vhpiAllocatorK,
    vhpiArchBodyK,
    vhpiArrayTypeDeclK,
    vhpiAssertStmtK,
    vhpiAssocElemK,
    vhpiAttrDeclK,
    vhpiAttrSpecK,
    /* The next is 1011*/
    vhpiBaseK,
    vhpiBinaryExprK,
    vhpiBitStringLiteralK,
    vhpiBlockConfigK,
    vhpiBlockStmtK,
    vhpiBranchK,
    vhpiCallbackK,
    vhpiCaseStmtK,
    vhpiCharLiteralK,
    vhpiCompConfigK,
    /* The next is 1021*/
    vhpiCompDeclK,
    vhpiCompInstStmtK,
    vhpiCompositeTypeDeclK,
    vhpiConcStmtK,
    vhpiCondSigAssignStmtK,
    vhpiCondWaveformK,
    vhpiConfigDeclK,
    vhpiConfigItemK,
    vhpiConstDeclK,
    vhpiConstParamDeclK,
    /* The next is 1031*/
    vhpiConversionK,
    vhpiDerefObjK,
    vhpiDesignInstUnitK,
    vhpiDesignUnitK,
    vhpiDriverK,
    vhpiElemDeclK,
    vhpiEntityDeclK,
    vhpiEnumLiteralK,
    vhpiEnumTypeDeclK,
    vhpiEqProcessStmtK,
    /* The next is 1041*/
    vhpiExitStmtK,
    vhpiFileDeclK,
    vhpiFileParamDeclK,
    vhpiFileTypeDeclK,
    vhpiFloatRangeK,
    vhpiFloatTypeDeclK,
    vhpiForGenerateK,
    vhpiForLoopStmtK,
    vhpiForeignfK,
    vhpiForGenStmtK,
    /* The next is 1051*/
    vhpiFuncCallK,
    vhpiFuncDeclK,
    vhpiGenStmtK,
    vhpiGenericDeclK,
    vhpiHandleK,
    vhpiIfGenerateK,
    vhpiIfGenStmtK,
    vhpiIfStmtK,
    vhpiIndexedAttrNameK,
    vhpiIndexedNameK,
    /* The next is 1061*/
    vhpiInPortDeclK,
    vhpiIntLiteralK,
    vhpiIntRangeK,
    vhpiIntTypeDeclK,
    vhpiIteratorK,
    vhpiNextStmtK,
    vhpiNullK,
    vhpiNullStmtK,
    vhpiOperatorK,
    vhpiOutPortDeclK,
    /* The next is 1071*/
    vhpiPackBodyK,
    vhpiPackDeclK,
    vhpiPackInstK,
    vhpiPhysLiteralK,
    vhpiPhysTypeDeclK,
    vhpiPliBundleK,
    vhpiPortDeclK,
    vhpiPredefSigAttrNameK,
    vhpiPrimaryUnitK,
    vhpiProcCallStmtK,
    /* The next is 1081*/
    vhpiProcDeclK,
    vhpiProcessStmtK,
    vhpiQualifiedExprK,
    vhpiRangeK,
    vhpiRealLiteralK,
    vhpiRecordTypeDeclK,
    vhpiRegionK,
    vhpiReportStmtK,
    vhpiReturnStmtK,
    vhpiRootInstK,
    /* The next is 1091*/
    vhpiScalarTypeDeclK,
    vhpiSecondaryUnitK,
    vhpiSelectSigAssignStmtK,
    vhpiSelectWaveformK,
    vhpiSelectedNameK,
    vhpiSeqSigAssignStmtK,
    vhpiSigDeclK,
    vhpiSigParamDeclK,
    vhpiSimpAttrNameK,
    vhpiSimpNameK,
    /* The next is 1101*/
    vhpiSliceNameK,
    vhpiStmtK,
    vhpiStringLiteralK,
    vhpiSubpCallK,
    vhpiSubtypeK,
    vhpiSubtypeDeclK,
    vhpiSubtypeIndicK,
    vhpiTimeQueueK,
    vhpiTransactionK,
    /* The next is 1111*/
    vhpiTypeDeclK,
    vhpiTypeMarkK,
    vhpiUnaryExprK,
    vhpiUnitDeclK,
    vhpiUserAttrNameK,
    vhpiVarAssignStmtK,
    vhpiVarDeclK,
    vhpiVarParamDeclK,
    vhpiWaitStmtK,
    vhpiWaveformElemK,
    /* The next is 1121*/
    vhpiWhileLoopStmtK,
    vhpiToolK,                   /* This will move up after ballot */
    vhpiPhysRangeK,               /* This again will move up after ballot */
    vhpiLoopStmtK,
    vhpiForLoopK,
    vhpiWhileLoopK,
    vhpiSimpleSigAssignStmtK,
    vhpiLoadK,
    vhpiFanoutK,
    vhpiDisconnectStmtK,
    /* The next is 1131*/
    vhpiProtectedTypeInstK,
    vhpiProtectedTypeDeclK,
    vhpiProtectedTypeBodyK
} vhpiClassKindT;

/* VHPI 1 to 1 relationships */
typedef enum {
    vhpiActual,
    vhpiAttrDecl,
    vhpiBaseType,
    vhpiBaseUnit,
    vhpiBasicSignal,
    vhpiBlockConfig,
    vhpiCaseExpr,
    vhpiCondExpr,
    vhpiConfigDecl,
    vhpiConfigSpec,
    vhpiCurProcess,
    vhpiCurStackFrame,
    vhpiDecl,
    vhpiDerefObj,
    vhpiDesignUnit,
    vhpiDownStack,
    vhpiDriver,
    vhpiElemSubtype,
    vhpiEntityAspect,
    vhpiEntityDecl,
    vhpiExpr,
    vhpiFormal,
    vhpiGuardSig,
    vhpiImmScope,
    vhpiInitExpr,
    vhpiInPort,
    vhpiLeftExpr,
    vhpiLocal,
    vhpiLogicalExpr,
    vhpiName,
    vhpiOperator,
    vhpiOutPort,
    vhpiParamDecl,
    vhpiParamExpr,
    vhpiParent,
    vhpiPhysLiteral,
    vhpiPortDecl,
    vhpiPrimaryUnit,
    vhpiProcessStmt,
    vhpiRange,
    vhpiRejectTime,
    vhpiReportExpr,
    vhpiResolFunc,
    vhpiReturnExpr,
    vhpiReturnTypemark,
    vhpiRhsExpr,
    vhpiRightExpr,
    vhpiSelectExpr,
    vhpiSeverityExpr,
    vhpiSigDecl,
    vhpiSigParamDecl,
    vhpiSimTimeUnit,
    vhpiSubpDecl,
    vhpiSubtype,
    vhpiSubtypeIndic,
    vhpiSuffix,
    vhpiTimeExpr,
    vhpiTimeOutExpr,
    vhpiTypeDecl,
    vhpiTypeMark,
    vhpiUpStack,
    vhpiUpperRegion,
    vhpiValExpr,
    vhpiValSubtype,
    vhpiPrefix,
    vhpiTool,                /* This will move up after ballot */
    vhpiLexicalScope,
    vhpiCurRegion,           /* new relation is added */
    vhpiLhsExpr,
    vhpiIterScheme,
    vhpiConstraint,
    vhpiProtectedTypeBody,
    vhpiProtectedTypeDecl,
    vhpiProtectedTypeInst,
    /* Le grand dike vhpiZDike, which prevents overflows beyond */
    vhpiZDike

} vhpiOneToOneT;

/* VHPI 1 to many relationships */
typedef enum {
    vhpiAliasDecls,
    vhpiAttrDecls,
    vhpiAttrSpecs,
    vhpiBasicSignals,
    vhpiBlockStmts,
    vhpiBranchs,
    vhpiCallbacks,
    vhpiChoices,
    vhpiCompInstStmts,
    vhpiCondExprs,
    vhpiCondWaveforms,
    /*-- next is the 11 th entry */
    vhpiConfigSpecs,
    vhpiConstDecls,
    vhpiConstraints,
    vhpiContributors,
    vhpiCurRegions,     /* This is mistake in STANDARD, relation isn't provided */
    vhpiDecls,
    vhpiDepUnits,
    vhpiDrivenSigs,
    vhpiDrivers,
    vhpiEntityDesignators,
    /*-- next is the 21 th entry */
    vhpiEnumLiterals,
    vhpiForeignfs,
    vhpiFormals,
    vhpiGenericAssocs,
    vhpiGenericDecls,
    vhpiHighConns,
    vhpiIndexConstraints,
    vhpiIndexExprs,
    vhpiIndexedNames,
    vhpiInternalRegions,
    /*-- next is the 31st entry */
    vhpiLowConns,
    vhpiPackInsts,
    vhpiParamAssocs,
    vhpiParamDecls,
    vhpiPliBundles,
    vhpiPortAssocs,
    vhpiPortDecls,
    vhpiRecordElems,
    vhpiRootInsts,
    vhpiSelectWaveforms,
    /*-- next is the 41st entry */
    vhpiSelectedNames,
    vhpiSensitivitys,
    vhpiSeqStmts,
    vhpiSigAttrs,
    vhpiSigDecls,
    vhpiSigNames,
    vhpiSources,
    vhpiSpecNames,
    vhpiStmts,
    vhpiTargets,
    /*-- next is the 51st entry */
    vhpiTimeQueues,
    vhpiTransactions,
    vhpiTypeMarks,
    vhpiUnitDecls,
    vhpiUses,
    vhpiVarDecls,
    vhpiWaveformElems,
    vhpiExprLeaves, /* non standard relation to iterate expession's leaves
                   * (signals, vars, ..., literals */
    vhpiExprWriters, /* non standard relation to iterate statement's
                    * writer expressions */
    vhpiExprReaders, /* non standard relation to iterate statement's
                    * reader expressions */
    /*-- next is the 61st entry */
    vhpiLoads,       /* non standard relation to iterate loads */
    vhpiFanouts,     /* non standard relation to iterate fanouts */
    vhpiActuals,
    vhpiFanoutsForForce,
    vhpiStmtDrivers, /* non standard relation to iterate statement drivers */
    vhpiLocalLoads,  /* LRM compliant relation to iterate local loads*/
    /* nothing below this should be added */
    vhpiZDikes

} vhpiOneToManyT;

/* VHPI Integer or Boolean Properties */
typedef enum {
    vhpiAccessP,
    vhpiAttrKindP,
    vhpiBeginLineNoP,
    vhpiEndLineNoP,
    vhpiForeignKindP,
    vhpiFrameLevelP,
    vhpiGenerateIndexP,
    vhpiInValueP,
    vhpiIsAnonymousP,
    vhpiIsBasicP,
    vhpiIsCompositeP,
    vhpiIsDefaultP,
    vhpiIsDeferredP,
    vhpiIsDiscreteP,
    vhpiIsForcedP,
    vhpiIsForeignP,
    vhpiIsGuardedP,
    vhpiIsImplicitDeclP,
    vhpiIsInfixP,
    vhpiIsNullP,
    vhpiIsOpenP,
    vhpiIsOthersP,
    vhpiIsPLIP,
    vhpiIsPassiveP,
    vhpiIsPostponedP,
    vhpiIsProtectedTypeP,
    vhpiIsPureP,
    vhpiIsResolutionP,
    vhpiIsResolvedP,
    vhpiIsScalarP,
    vhpiIsSharedP,
    vhpiIsTransportP,
    vhpiIsUnaffectedP,
    vhpiIsUnconstrainedP,
    vhpiIsUpP,
    vhpiIsVitalP,
    vhpiKindP,
    vhpiLanguageP,
    vhpiLeftBoundP,
    vhpiLineNoP,
    vhpiLineOffsetP,
    vhpiLoopIndexP,
    vhpiModeP,
    vhpiVssNameClassP,
    vhpiNumDimensionsP,
    vhpiNumFieldsP,
    vhpiNumLiteralsP,
    vhpiNumParamsP,
    vhpiOpenModeP,
    vhpiOutValueP,
    vhpiPositionP,
    vhpiPrecisionP,
    vhpiPredefAttrP,
    vhpiProtectedLevelP,
    vhpiReasonP,
    vhpiRightBoundP,
    vhpiSigKindP,
    vhpiSimTimeP,
    vhpiSimTimeUnitP,
    vhpiSizeP,
    vhpiStartLineNoP,
    vhpiStateP,
    vhpiLevelP,
    vhpiVHDLversionP,
    vhpiPhaseP,
    vhpiToolVersionP,
    vhpiIsObjectReadableP,
    vhpiIsObjectWriteableP,
    vhpiIsAnyBitForcedP,
    vhpiLPKindP,
    vhpiIsAnyBitPliForcedP,

    /* new properties should only be added above this one here */
    vhpiZDikeIntP

} vhpiIntPropertyT;

/* VHPI Real Properties */
typedef enum {
    vhpiFloatLeftBoundP,
    vhpiFloatRightBoundP,

    vhpiZDikeRealP
} vhpiRealPropertyT;

/* VHPI Physical Properties */
typedef enum {
    vhpiPhysPositionP,
    vhpiPhysLeftBoundP,
    vhpiPhysRightBoundP,

    vhpiZDikePhysP
} vhpiPhysPropertyT;

/* VHPI String Properties */
typedef enum {
    vhpiCaseNameP,
    vhpiCompNameP,
    vhpiDefNameP,
    vhpiFileNameP,
    vhpiFullCaseNameP,
    vhpiFullNameP,
    vhpiKindStrP,
    vhpiLabelNameP,
    vhpiLibLogicalNameP,
    vhpiLibPhysicalNameP,
    vhpiLogicalNameP,
    vhpiNameP,
    vhpiOpNameP,
    vhpiStrValP,
    vhpiUnitNameP,
    vhpiIncompatibleVersionP,    /* Non-standard */
    vhpiSimulatorVersionP,       /* Non-standard */
    vhpiDecompileP,             /* Non-standard */
    vhpiFullFileNameP,          /* Non-standard */

    vhpiZDikeStrP
} vhpiStrPropertyT;

/* VHPI Value Formats */
typedef enum {
    vhpiEnumVal,
    vhpiEnumVecVal,
    vhpiIntVal,
    vhpiIntVecVal,
    vhpiRealVal,
    vhpiRealVecVal,
    vhpiPhysVal,
    vhpiPhysVecVal,
    vhpiCharVal,
    vhpiStrVal,
    vhpiObjTypeVal,
    vhpiPtrVal,                /* for access type variables */
    vhpiPtrVecVal,             /* the ptr formats will have different */
    /* positional values after ballot */
    vhpiRawData,

    /* The following format value is for internal use by the VHPI
       implementation of Synopsys */

    vhpiInvalidFormat

} vhpiValueFormatT;

/* VHPI Callback Reasons */
typedef enum {
    vhpiCbValueChange,              /* Variables in debug mode */
    vhpiCbForce,                    /* not supported */
    vhpiCbRelease,                  /* not supported */
    vhpiCbSensitivitySet,           /* Non-standard. Will disappear soon */

    vhpiCbStmt,                     /* not supported */
    vhpiCbResume,                   /* not supported */
    vhpiCbSuspend,                  /* not supported */
    vhpiCbStartOfSubpCall,
    vhpiCbEndOfSubpCall,            /* not supported */
    vhpiCbTransaction,              /* not supported for drivers */

    vhpiCbNextTimeStep,
    vhpiCbAfterDelay,

    vhpiCbStartOfNextCycle,
    vhpiCbEndOfPropagation,
    vhpiCbEndOfProcesses,
    vhpiCbLastDeltaCycle,           /* Non-standard. Will remain */
    vhpiCbStartOfPostponed,
    vhpiCbEndOfTimeStep,

    vhpiCbElaboration,
    vhpiCbInitialization,
    vhpiCbStartOfSimulation,
    vhpiCbEndOfSimulation,
    vhpiCbQuiescence,

    vhpiCbPLIError,
    vhpiCbEnterInteractive,         /* not supported */
    vhpiCbExitInteractive,          /* not supported */
    vhpiCbSigInterrupt,             /* not supported */
    vhpiCbStartOfSave,
    vhpiCbEndOfSave,
    vhpiCbStartOfRestart,           /* not supported */
    vhpiCbEndOfRestart,
    vhpiCbStartOfReset,
    vhpiCbEndOfReset,               /* not supported */

    vhpiCbActive,                   /* Non-standard, use vhpiCbTransaction */
    vhpiCbStartOfProcesses          /* Will move up after ballot */

} vhpiCbReasonT;

/* VHPI Callback State */
typedef enum {
    vhpiCbStateEnabled,
    vhpiCbStateDisabled,
    vhpiCbStateMatured
} vhpiCbStateT;

/* VHPI Value Data Structure */
typedef struct {
    vhpiValueFormatT format;   /* value format */
    vhpiIntT bufSize;          /* size of the vector */
    vhpiIntT numscalars;       /* the number of scalar represented by the value */
    vhpiIntT unit;             /* physical unit. 0 for base unit */
    union {
        vhpiEnumT enumval, *enums; /* enumeration value(s) */
        vhpiIntT  intg, *intgs;    /* integer value(s) */
        vhpiRealT real, *reals;    /* real value(s) */
        vhpiPhysT phys, *physs;    /* physical value(s) */
        char      ch, *str;        /* character or string formatting */
        void      *ptr, **ptrs;         /* values of access types and raw data */
    } value;
} vhpiValueT;

/* VHPI Callback Data Structure */
typedef struct vhpiCbDataS {
    vhpiCbReasonT reason;             /* callback reason */
    void (*cbf)(struct vhpiCbDataS*); /* callback function */
    vhpiHandleT   obj;                /* trigger object */
    vhpiTimeT*    time;               /* callback time */
    vhpiValueT*   value;              /* trigger object value */
    void*         user_data;          /* to be passed to the callback function */
} vhpiCbDataT;

/* Severity */
typedef enum {
    vhpiNote,
    vhpiMessage,
    vhpiWarning,
    vhpiError,
    vhpiException,
    vhpiFatal,
    vhpiInternal
} vhpiSeverityT;

/* Error information structure */
typedef struct vhpiErrorInfoS
{
    vhpiSeverityT severity;           /* The severity level of the error */
    char* message;                    /* The error message string */
    const char* str;                        /* The error code */
    char* file;                       /* Name of the VHDL file where the
                                         error originated */
    int line;                         /* The line number in the VHDL file
                                         of the item related to the error */

} vhpiErrorInfoT;

/* Put Value Flags. The string literals will change with grand unification
   after the standard is balloted */

/* Standard support:
 * -----------------
 *
 * vhpiForce:
 * This assigns(puts/deposits) value with no value propogation.
 * Any previous Hold is purged.
 *
 * vhpiForcePropogate:
 * This assigns(puts/deposits) and propogates the value in next
 * delta cycle. Any previous Hold is purged.
 *
 * vhpiForceHold:
 * This holds(freezes) the value with no value propogation. Any
 * previous Hold is purged.
 *
 * vhpiForceHoldPropogate:
 * This holds(freezes) and propogates the value in next
 * delta cycle. Any previous Hold is purged.
 *
 */


/* Non standard support:
 * ---------------------
 * Since standard satisfying propogate flags are executed in next delta
 * cycle, following non-standard flags for executing in current delta
 * cycle are provided for user convinience.
 *
 * vhpiForcePropogateImmediate or vhpiForceImmediate:
 * (assign and propogate value immediately with existing Hold purged)
 * Same as vhpiForce plus value propogation in current delta cycle
 * and with an exception; a previous Hold on it can over-ride it.
 *
 * vhpiForcePropogateImmediatePurge or vhpiForceImmediatePurge:
 * (assign and propogate value immediately purging any existing Hold)
 * Same as vhpiForcePropogateImmediate except any previous Hold is purged.
 *
 * vhpiForceHoldImmediate:
 * (hold value immediately purging any previous hold)
 * Same as vhpiForceHold except value is held in current delta cycle.
 * Any previous Hold is purged.
 *
 * vhpiForceHoldPropogateImmediate:
 * (hold and propogate value immediately purging any previous hold)
 * Same as vhpiForceHoldPropogate except value is held and propogated
 * in current delta cycle. Any previous Hold is purged.
 *
 */


typedef enum {
    vhpiForce,
    vhpiForcePropagate,
    vhpiForceHold,
    vhpiForceHoldPropagate,

    /* NOTE: Following are non-standard flags added for
     * user convinience. See above for their description.
     */
    vhpiForceImmediate,
    vhpiForcePropogateImmediate = vhpiForceImmediate,
    vhpiForceImmediatePurge,
    vhpiForcePropogateImmediatePurge = vhpiForceImmediatePurge,
    vhpiForceHoldImmediate,
    vhpiForceHoldPropogateImmediate = vhpiForceHoldImmediate,
    vhpiForce_MaxFlagCount      /* To track the # of flags supported */
} vhpiPutValueFlagsT;

/* Signal kind */
typedef enum {
    vhpiRegular,
    vhpiBus,
    vhpiRegister
} vhpiSigKindT;

/* Interface mode */
typedef enum {
    vhpiIn,
    vhpiOut,
    vhpiInout,
    vhpiBuffer,
    vhpiLinkage
} vhpiModeT;

/* VHPI control flags enum type */
typedef enum {
    vhpiFinish,
    vhpiStop,
    vhpiReset
} vhpiControlFlagsT;

/* VHPI supported languages enumeration type */
typedef enum {
    vhpiVHDL,
    vhpiVerilog
} vhpiLanguageT;

/* Enumeration type for the literals of TIME */
typedef enum {
    vhpiFS,
    vhpiPS,
    vhpiNS,
    vhpiUS,
    vhpiMS,
    vhpiSEC,
    vhpiMIN,
    vhpiHR
} vhpiTimeUnitT;

/* Enumeration type for the delay mode */
typedef enum {
    vhpiInertial,
    vhpiTransport
} vhpidelayModeT;


/* Predefined Attribute types returned by vhpiPredefAttrP */
typedef enum {
    vhpiActivePA = 1001,
    vhpiAscendingPA = 1002,
    vhpiBasePA = 1003,
    vhpiDelayedPA = 1004,
    vhpiDrivingPA = 1005,
    vhpiDriving_valuePA = 1006,
    vhpiEventPA = 1007,
    vhpiHighPA = 1008,
    vhpiImagePA = 1009,
    vhpiInstance_namePA = 1010,
    vhpiLast_activePA = 1011,
    vhpiLast_eventPA = 1012,
    vhpiLast_valuePA = 1013,
    vhpiLeftPA = 1014,
    vhpiLeftofPA = 1015,
    vhpiLengthPA = 1016,
    vhpiLowPA = 1017,
    vhpiPath_namePA = 1018,
    vhpiPosPA = 1019,
    vhpiPredPA = 1020,
    vhpiQuietPA = 1021,
    vhpiRangePA = 1022,
    vhpiReverse_rangePA = 1023,
    vhpiRightPA = 1024,
    vhpiRightofPA = 1025,
    vhpiSimple_namePA = 1026,
    vhpiStablePA = 1027,
    vhpiSuccPA = 1028,
    vhpiTransactionPA = 1029,
    vhpiValPA = 1030,
    vhpiValuePA = 1031
} vhpiPredefAttrT;

/**************************** CALLBACK FLAGS ******************************/
#define vhpiReturnCb  0x00000001
#define vhpiDisableCb 0x00000010

/* The following are the VHPI ANSI C functions */

/* vhpi_assert
   The following function is equivalent to a VHDL assert statement.
   The function returns 0 on success and 1 on failure */

int vhpi_assert(char* msg, vhpiSeverityT severity);

/* vhpi_compare_handles
   This function can be used to compare two PLI handles. It returns
   1 if they are the same and 0 otherwise */

int vhpi_compare_handles(vhpiHandleT lhs, vhpiHandleT rhs);

/* vhpi_create
   This function can be used to create processes or signal drivers
   depending on the kind argument.  The legal values of kind are:
       - vhpiProcessStmtK to create a process
       - vhpiDriverK to create a signal driver
       - vhpiDriverCollectionK to create a driver collection
   It returns a handle to the newly created process or driver */

vhpiHandleT vhpi_create(vhpiClassKindT kind, vhpiHandleT refHandle, vhpiHandleT processHandle);

/* vhpi_disable_cb
   This function can be used to disable a registered callback. If the
   callback has already matured, calling this function amounts to a
   no-op */

int vhpi_disable_cb(vhpiHandleT cbHandle);

/* vhpi_enable_cb
   This function can be used to enable a registered callback. If the
   callback is the enabled state, this amounts to a no-op, if it is
   disabled, by a call to vhpi_disable, it gets enabled by this call.
   If the callback has matured, then this call re-enables the callback
 */

int vhpi_enable_cb(vhpiHandleT cbHandle);

/* vhpi_get
   This function can be used to get integer valued properties. This
   function returns 0 on failure, or the value of the property */

vhpiIntT vhpi_get(vhpiIntPropertyT property, vhpiHandleT handle);

/* vhpi_get_cb_info
   This function can be used to get callback information for a
   registered callback. The callback handle and a user allocated
   callback structure are parameters. Function return 0 on success
   and 1 on failure */

int vhpi_get_cb_info(vhpiHandleT cbHandle, vhpiCbDataT* pCbData);

/* vhpi_get_str
   This function can be used to return string valued properties.
   The return value is 0 on failure. On success, the string value
   that is returned is expected to copied before a subsequent
   call to this function as the PLI does not guarantee persistence
   across calls */

char* vhpi_get_str(vhpiStrPropertyT property, vhpiHandleT handle);

/* vhpi_get_real
   This function can be used to get real valued properties. It
   returns 0 on failure */

vhpiRealT vhpi_get_real(vhpiRealPropertyT property, vhpiHandleT handle);

/* vhpi_get_phys
   This function can be used to get physical valued properties. It
   returns 0 on failure */

vhpiPhysT vhpi_get_phys(vhpiPhysPropertyT property, vhpiHandleT handle);

/* vhpi_get_time
   This function can be used to get the current simulation time. The
   time is a long long value and the current delta is returned in the
   cycles parameter. The time units are the current simulation time
   units */

#define vhpiNoActivity -1

void vhpi_get_time(vhpiTimeT* time, unsigned long* cycles);

/* vhpi_get_next_time
   This function can be used to get the next active simulation time.
   The function shall return the next active time if this function is
   called during posponded process or end of time phase and current
   time when called in any other phase. The function returns 0 when
   there is a next scheduled time, and the time argument provides
   the absolute time value for the next event. If no event scheduled,
   the time low and high fields should be both set to represent the
   value of Time`HIGH and the function should return vhpiNoActivity
   (defined to be the constant 1). If there is any error during the
   execution of this function, the function returns a non zero value
   (other than vhpiNoActivity), the time value is unspecified. This
   function can be called at the end of time step or at end of
   initialization. */

int vhpi_get_next_time(vhpiTimeT* time);

/* vhpi_get_value
   This function can be used to get the value of a VHDL object. VHDL
   objects would be signals, variables, constants, generics and
   ports. The function returns 0 on success and 1 on failure. The
   value structure is expected to be allocated by the user in addition
   to the internal buffer in the case of values of composites */

int vhpi_get_value(vhpiHandleT objectHandle, vhpiValueT* pValue);


int vhpi_get_value_std(vhpiHandleT objectHandle, vhpiValueT* pValue);

/* vhpi_handle
   This function can be used to traverse one-to-one relationships.
   The relation and the reference object are passed as parameters.
   If the requested relationship is not supported with respect to
   the class type of the reference handle, then an error will be
   generated */

vhpiHandleT vhpi_handle(vhpiOneToOneT relation, vhpiHandleT handle);

/* vhpi_handle_by_index
   This function can be used to get an object corresponding to the
   passed index in an implicit iterator for the one-to-many
   relationship identified by relation, with respect to the class
   of the reference handle */

vhpiHandleT vhpi_handle_by_index(vhpiOneToManyT relation,
                                 vhpiHandleT handle, int index);

/* vhpi_handle_by_name
   This function can be used to perform handle searches by name. The
   name passed here is expected to be in the same format as the VHDL
   path_name, with the difference being that it could be absolute or
   relative to the reference object handle passed */

vhpiHandleT vhpi_handle_by_name(char* name, vhpiHandleT handle);

/* vhpi_iterator
   This function can be used to traverse one-to-many relationships.
   The relation identifies the relationship and the handle should
   have a class type that supports that relationship */

vhpiHandleT vhpi_iterator(vhpiOneToManyT relation, vhpiHandleT handle);

/* vhpi_fileLineInstIterator
   This function can be used to traverse one-to-many relationships
   of a file/line pair to instances */

vhpiHandleT vhpi_fileLineInstIterator (const char *fileName, unsigned int lineNo);

/* vhpi_printf
   This function can be used to print a message out on standard output */

int vhpi_printf(const char* format, ...);

/* vhpi_put_value
   This function can be used to change the value of certain VHDL object.
   VHDL objects in this context are signals, variables and ports */

int vhpi_put_value(vhpiHandleT objectHandle,
                   vhpiValueT* pValue, vhpiPutValueFlagsT flags);

/* vhpi_register_cb
   This function can be used to register a callback. The callback data
   is passed through the user allocated callback data structure */

vhpiHandleT vhpi_register_cb(vhpiCbDataT* pCbData);

/* vhpi_register_cb_std
LRM version declaration of vhpi_register_cb, but still need alloc mem by user */

vhpiHandleT vhpi_register_cb_std(vhpiCbDataT* pCbData, unsigned int flags);

/* vhpi_register_cb_std_all
LRM version declaration of vhpi_register_cb*/
vhpiHandleT vhpi_register_cb_std_all(vhpiCbDataT* pCbData, unsigned int flags);


/* vhpi_release_handle
   This function should be used to release a PLI handle. All handles
   are created by the PLI and should be released by the users to
   prevent potential memory leaks */

int vhpi_release_handle(vhpiHandleT handle);

/* vhpi_remove_cb
   This function can be used to remove a registered callback. The
   handle to the callback is passed */

int vhpi_remove_cb(vhpiHandleT cbHandle);

/* vhpi_scan
   This function can be used to scan through an iterator. The iterator
   creation and this function constitute the PLI mechanism to traverse
   one-to-many relationships */

vhpiHandleT vhpi_scan(vhpiHandleT iterator);

/* vhpi_reset
   This function can be used to reset an iterator. The iterator should
   be to a static post elaboration set. In other words, iterators to
   driver transactions (not in Phase I) or registered callbacks are
   considered dynamic lists to which this function does not apply.
   Once reset an iterator handle can be reused with vhpi_scan starting
   out at the beginning of the list */

int vhpi_reset(vhpiHandleT iterator);

/* vhpi_sim_control
   This function can be used to issue a control command to the simulator.
   This could be one of the values defined in vhpiControlFlagsT (vhpi_user.h) */

int vhpi_sim_control(vhpiControlFlagsT control);

/* vhpi_chk_error
   This function can be used to check errors. Each call to this function
   with a user allocated error data structure will return one error
   from a queue of errors that were encountered when the last PLI
   function was executed */

int vhpi_chk_error(vhpiErrorInfoT* pError);

/* vhpi_value_size
   This function can be used to get the size in bytes of an object
   value, given the format in which a subsequent value related method
   will request a value or set a value. This should be used to
   allocate the value buffer in the value structure */

vhpiIntT vhpi_value_size(vhpiHandleT objectHandle, vhpiValueFormatT format);

/* vhpi_release_hold
   This function can be used to release a signal or port that is on hold.
   A signal/port can be placed on hold by choosing vhpiForce or
   vhpiForce | vhpiForceEvent for the vhpi_put_value flags parameter */

vhpiIntT vhpi_release_hold(vhpiHandleT objectHandle);

/* vhpi_schedule_transaction
   This can be used to schedule transaction on driver. The function
   can only be called during process execution phase. The transaction
   can be scheduled with zero or non-zero delay and with internal or
   transport mode. To schedule a value on driver requires to get handle
   (objectHandle) to a driver of signal, specify a value pointer (pValue),
   the number of values (numValues), a delay (pDelay) and
   delay (delayMode) mode. The delay modes that are supported vhpiInertialT
   and vhpiTransportT. In the case of inertial delays, a user could specify
   an additional optional parameter, the pulse rejection limit that should
   be passed in pPulseRej parameter.
   Returns 0 on success, non zero on
*/


int vhpi_schedule_transaction(vhpiHandleT objectHandle,
                              vhpiValueT** pValue,
                              int numValues,
                              vhpiTimeT* pDelay,
                              int delayMode,
                              vhpiTimeT* pPulseRej);

/* This function is a utility function that can be used to clone (copy)
   a vhpi handle. Currently used in MVhpiDriver.C an MVhpiLoad.C */

void vhpi_handle_clone(vhpiHandleT objectHandle);

/*  vhpi_is_access_type
    Specify if the handle is ACCESS type */
int vhpi_is_access_type(vhpiHandleT objectHandle);

typedef int (*vhpiUserFctT) ();
int vhpi_protected_call (vhpiHandleT varHdl,
                         vhpiUserFctT userFct,
                         void *userData);

extern int g_findForVITAL2K;
extern char *gGetSimvPath(void);
/*
The macro VCS_VHPI_INT_COMPILE is defined in vgbuild,
and both functions will not be aliased.
This is to keep the original usage of internal call
*/
#ifdef STD_VCS_VHPI
#   ifdef OLD_VCS_VHPI
#       error "Both OLD_VCS_VHPI and STD_VCS_VHPI preprocessor symbols defined, Only define one."
#   else
#       define vhpi_register_cb vhpi_register_cb_std
#       define vhpi_get_value vhpi_get_value_std
#   endif
#else
#   ifndef OLD_VCS_VHPI
#       define vhpi_get_value vhpi_get_value_std
#       define vhpi_register_cb vhpi_register_cb_std_all
#   endif
#endif


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
#endif
