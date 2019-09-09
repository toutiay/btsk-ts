import { Task } from "./Task";
import { Default } from "../Enum";

export class Node {

    create(): Task {
        return Default.UNDEFINED;
    }

    destroy(t: Task): void {

    }
}