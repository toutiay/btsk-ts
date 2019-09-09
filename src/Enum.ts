export enum Status {
    BH_INVALID,
    BH_SUCCESS,
    BH_FAILURE,
    BH_RUNNING,
    BH_ABORTED,
    BH_SUSPENDED,
}

export enum Policy {
    RequireOne,
    RequireAll,
}

export class Default {
    static UNDEFINED = [][0];
    static NULL = [][0];
}