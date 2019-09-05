"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Task_1 = require("./Task");
const Default_1 = require("../Enum/Default");
const Behavior_1 = require("./Behavior");
const Status_1 = require("../Enum/Status");
class Selector extends Task_1.Task {
    constructor(node) {
        super(node);
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = Default_1.Default.UNDEFINED;
        this.m_Behavior = new Behavior_1.Behavior(Default_1.Default.UNDEFINED);
    }
    getNode() {
        return this.m_pNode;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
        this.m_Behavior.setup(this.m_CurrentChild);
    }
    update() {
        while (true) {
            let s = this.m_Behavior.tick();
            if (s != Status_1.Status.BH_FAILURE) {
                return s;
            }
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.getNode().m_Children.length) {
                return Status_1.Status.BH_FAILURE;
            }
            this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
            this.m_Behavior.setup(this.m_CurrentChild);
        }
    }
}
exports.Selector = Selector;
//# sourceMappingURL=Selector.js.map