import { Behavior } from "./Behavior";
import { Parallel } from "./Parallel";
import { Policy } from "../Enum";

export class Monitor extends Parallel {
    constructor() {
        super(Policy.RequireOne, Policy.RequireOne);
    }

    addCondition(condition: Behavior) {
        this.m_Children.unshift(condition);
    }

    addAction(action: Behavior) {
        this.m_Children.push(action);
    }
}