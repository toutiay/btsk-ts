import { Status } from './../Enum/Status';
import { Node } from './Node';
export class Task {
    m_pNode: Node;
    constructor(node: Node) {
        this.m_pNode = node;
    }

    update(): number {
        return Status.BH_INVALID;
    }

    onInitialize() { }

    onTerminate(s: number) { }
}