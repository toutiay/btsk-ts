"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
class Composite extends Behavior_1.Behavior {
    constructor() {
        super(...arguments);
        this.m_Children = [];
    }
    addChild(child) {
        this.m_Children.push(child);
    }
    removeChild(child) {
        let index = this.m_Children.indexOf(child);
        if (index != -1) {
            this.m_Children.splice(index, 1);
        }
    }
    clearChildren() {
        this.m_Children.length = 0;
    }
}
exports.Composite = Composite;
//# sourceMappingURL=Composite.js.map