"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const Test_1 = __importDefault(require("./Test/Test"));
Test_1.default.TEST_TaskInitialize();
Test_1.default.TEST_TaskUpdate();
Test_1.default.TEST_TaskTerminate();
Test_1.default.TEST_SequenceTwoChildrenFails();
Test_1.default.TEST_SequenceTwoChildrenContinues();
Test_1.default.TEST_SequenceOneChildPassThrough();
Test_1.default.TEST_SelectorTwoChildrenContinues();
Test_1.default.TEST_SelectorTwoChildrenSucceeds();
Test_1.default.TEST_SelectorOneChildPassThrough();
Test_1.default.TEST_ParallelSucceedRequireAll();
Test_1.default.TEST_ParallelSucceedRequireOne();
Test_1.default.TEST_ParallelFailureRequireAll();
Test_1.default.TEST_ParallelFailureRequireOne();
Test_1.default.TEST_ActiveBinarySelector();
// Test.TEST_TaskInitialize();
// Test.TEST_TaskUpdate();
// Test.TEST_TaskTerminate();
//# sourceMappingURL=index.js.map