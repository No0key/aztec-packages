import type {
  ABIParameter,
  ABIParameterVisibility,
  AbiErrorType,
  AbiType,
  AbiValue,
  DebugFileMap,
  DebugInfo,
} from '../abi/abi.js';

export const AZTEC_PRIVATE_ATTRIBUTE = 'private';
export const AZTEC_PUBLIC_ATTRIBUTE = 'public';
export const AZTEC_UTILITY_ATTRIBUTE = 'utility';
export const AZTEC_INTERNAL_ATTRIBUTE = 'internal';
export const AZTEC_INITIALIZER_ATTRIBUTE = 'initializer';
export const AZTEC_VIEW_ATTRIBUTE = 'view';

/** The ABI of an Aztec.nr function. */
export interface NoirFunctionAbi {
  /** The parameters of the function. */
  parameters: ABIParameter[];
  /** The return type of the function. */
  return_type: {
    /**
     * The type of the return value.
     */
    abi_type: AbiType;
    /**
     * The visibility of the return value.
     */
    visibility: ABIParameterVisibility;
  };
  /** Mapping of error selector => error type */
  error_types: Partial<Record<string, AbiErrorType>>;
}

/**
 * The compilation result of an Aztec.nr function.
 */
interface NoirFunctionEntry {
  /** The name of the function. */
  name: string;
  /** Whether the function is unconstrained. */
  is_unconstrained: boolean;
  /** Custom attributes attached to function */
  custom_attributes: string[];
  /** The ABI of the function. */
  abi: NoirFunctionAbi;
  /** The bytecode of the function in base64. */
  bytecode: string;
  /** The proving key. */
  proving_key?: string;
  /** The verification key. */
  verification_key?: string;
  /** The debug information, compressed and base64 encoded. */
  debug_symbols: string;
  /** Map opcode index to assert message for public functions */
  assert_messages?: Record<number, string>;
}

/**
 * The compilation result of an Aztec.nr contract.
 */
export interface NoirCompiledContract {
  /** The name of the contract. */
  name: string;
  /** The functions of the contract. */
  functions: NoirFunctionEntry[];
  /** The events of the contract */
  outputs: {
    structs: Record<string, AbiType[]>;
    globals: Record<string, AbiValue[]>;
  };
  /** The map of file ID to the source code and path of the file. */
  file_map: DebugFileMap;
}

/**
 * The compilation result of a protocol (non-contract) circuit.
 */
export interface NoirCompiledCircuit {
  /** The hash of the circuit. */
  hash?: number;
  /**
   * The ABI of the function.
   */
  abi: NoirFunctionAbi;
  /** The bytecode of the circuit in base64. */
  bytecode: string;
  /** The debug information, compressed and base64 encoded. */
  debug_symbols: string;
  /** The map of file ID to the source code and path of the file. */
  file_map: DebugFileMap;
}

export interface NoirCompiledCircuitWithName extends NoirCompiledCircuit {
  /** The name of the circuit. */
  name: string;
}

/**
 * The debug metadata of an Aztec.nr contract.
 */
export interface NoirDebugMetadata {
  /**
   * The debug information for each function.
   */
  debug_symbols: DebugInfo[];
  /**
   * The map of file ID to the source code and path of the file.
   */
  file_map: DebugFileMap;
}

/**
 * The compilation artifacts of a given contract.
 */
export interface NoirContractCompilationArtifacts {
  /**
   * The compiled contract.
   */
  contract: NoirCompiledContract;
}

/**
 * The compilation artifacts of a given program.
 */
export interface NoirProgramCompilationArtifacts {
  /**
   * not part of the compilation output, injected later
   */
  name: string;
  /**
   * The compiled contract.
   */
  program: NoirCompiledCircuit;
}

/**
 * output of Noir Wasm compilation, can be for a contract or lib/binary
 */
export type NoirCompilationResult = NoirContractCompilationArtifacts | NoirProgramCompilationArtifacts;

/**
 * Check if it has Contract unique property
 */
export function isNoirContractCompilationArtifacts(
  artifact: NoirCompilationResult,
): artifact is NoirContractCompilationArtifacts {
  return (artifact as NoirContractCompilationArtifacts).contract !== undefined;
}

/**
 * Check if it has Contract unique property
 */
export function isNoirProgramCompilationArtifacts(
  artifact: NoirCompilationResult,
): artifact is NoirProgramCompilationArtifacts {
  return (artifact as NoirProgramCompilationArtifacts).program !== undefined;
}
