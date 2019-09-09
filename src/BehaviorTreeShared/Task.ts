import { Node } from './Node';
import { Status } from '../Enum';
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