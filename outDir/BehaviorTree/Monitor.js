"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Parallel_1 = require("./Parallel");
const Enum_1 = require("../Enum");
class Monitor extends Parallel_1.Parallel {
    constructor() {
        super(Enum_1.Policy.RequireOne, Enum_1.Policy.RequireOne);
    }
    addCondition(condition) {
        this.m_Children.unshift(condition);
    }
    addAction(action) {
        this.m_Children.push(action);
    }
}
exports.Monitor = Monitor;
//# sourceMappingURL=Monitor.js.map