"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Default_1 = require("./../Enum/Default");
const Task_1 = require("./Task");
const Status_1 = require("../Enum/Status");
class Sequence extends Task_1.Task {
    constructor(node) {
        super(node);
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = Default_1.Default.UNDEFINED;
        this.m_CurrentBehavior = Default_1.Default.UNDEFINED;
    }
    getNode() {
        return this.m_pNode;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
        this.m_CurrentBehavior.setup(this.m_CurrentChild);
    }
    update() {
        while (true) {
            let s = this.m_CurrentBehavior.tick();
            if (s != Status_1.Status.BH_SUCCESS) {
                return s;
            }
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.getNode().m_Children.length) {
                return Status_1.Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
            this.m_CurrentBehavior.setup(this.m_CurrentChild);
        }
    }
}
exports.Sequence = Sequence;
//# sourceMappingURL=Sequence.js.map