"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Task_1 = require("./Task");
const Behavior_1 = require("./Behavior");
const Enum_1 = require("../Enum");
class Sequence extends Task_1.Task {
    constructor(node) {
        super(node);
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = Enum_1.Default.UNDEFINED;
        this.m_CurrentBehavior = new Behavior_1.Behavior(Enum_1.Default.UNDEFINED);
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
            if (s != Enum_1.Status.BH_SUCCESS) {
                return s;
            }
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.getNode().m_Children.length) {
                return Enum_1.Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
            this.m_CurrentBehavior.setup(this.m_CurrentChild);
        }
    }
}
exports.Sequence = Sequence;
//# sourceMappingURL=Sequence.js.map