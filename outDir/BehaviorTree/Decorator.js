"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
class Decorator extends Behavior_1.Behavior {
    constructor(child) {
        super();
        this.m_pChild = child;
    }
}
exports.Decorator = Decorator;
//# sourceMappingURL=Decorator.js.map