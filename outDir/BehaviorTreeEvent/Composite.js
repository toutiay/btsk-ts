"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
class Composite extends Behavior_1.Behavior {
    constructor() {
        super(...arguments);
        this.m_Children = [];
    }
}
exports.Composite = Composite;
//# sourceMappingURL=Composite.js.map