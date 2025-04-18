import { Fr } from '@aztec/foundation/fields';

import { FunctionSelector } from '../abi/function_selector.js';
import { computeContractClassId } from './contract_class_id.js';
import type { ContractClass } from './interfaces/contract_class.js';

describe('ContractClass', () => {
  describe('getContractClassId', () => {
    it('calculates the contract class id', async () => {
      const contractClass: ContractClass = {
        version: 1,
        artifactHash: Fr.fromHexString('0x1234'),
        packedBytecode: Buffer.from('123456789012345678901234567890', 'hex'),
        privateFunctions: [
          {
            selector: FunctionSelector.fromString('0x12345678'),
            vkHash: Fr.fromHexString('0x1234'),
          },
        ],
      };
      const contractClassId = await computeContractClassId(contractClass);

      expect(contractClassId.toString()).toMatchInlineSnapshot(
        `"0x2c3a8b2ad29dd4000cb827e973737bcf57fc072aeaf93ceeef4b4b9eb086cf67"`,
      );
    });
  });
});
