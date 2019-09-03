"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const BehaviorTree_1 = __importDefault(require("./BehaviorTree"));
BehaviorTree_1.default.TEST_TaskInitialize();
BehaviorTree_1.default.TEST_TaskUpdate();
BehaviorTree_1.default.TEST_TaskTerminate();
BehaviorTree_1.default.TEST_SequenceTwoChildrenFails();
BehaviorTree_1.default.TEST_SequenceTwoChildrenContinues();
BehaviorTree_1.default.TEST_SequenceOneChildPassThrough();
BehaviorTree_1.default.TEST_SelectorTwoChildrenContinues();
BehaviorTree_1.default.TEST_SelectorTwoChildrenSucceeds();
BehaviorTree_1.default.TEST_SelectorOneChildPassThrough();
BehaviorTree_1.default.TEST_ParallelSucceedRequireAll();
BehaviorTree_1.default.TEST_ParallelSucceedRequireOne();
BehaviorTree_1.default.TEST_ParallelFailureRequireAll();
BehaviorTree_1.default.TEST_ParallelFailureRequireOne();
BehaviorTree_1.default.TEST_ActiveBinarySelector();
//# sourceMappingURL=index.js.map