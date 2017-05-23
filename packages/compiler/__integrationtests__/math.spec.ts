async function pi() {
    "use speedyjs";

    return Math.PI;
}

async function sqrt(value: number) {
    "use speedyjs";

    return Math.sqrt(value);
}

async function pow(base: number, exp: number) {
    "use speedyjs";

    return Math.pow(base, exp);
}

async function log(value: number) {
    "use speedyjs";

    return Math.log(value);
}

async function sin(value: number) {
    "use speedyjs";

    return Math.sin(value);
}

async function cos(value: number) {
    "use speedyjs";

    return Math.cos(value);
}

describe("Math", () => {
    describe("PI", () => {
        it("returns the value PI", async (cb) => {
            expect(await pi()).toBe(Math.PI);
            cb();
        });
    });

    describe("sqrt", () => {
        it("computes the square root", async (cb) => {
            expect(await sqrt(23.33)).toBe(Math.sqrt(23.33));
            cb();
        });
    });

    describe("pow", () => {
        it("computes the power of the base to the exponent", async (cb) => {
            expect(await pow(2.34, 34.2)).toBe(Math.pow(2.34, 34.2));
            cb();
        });
    });

    describe("log", () => {
        it("computes the log", async (cb) => {
            expect(await log(23.33)).toBe(Math.log(23.33));
            cb();
        });
    });

    describe("sin", () => {
        it("computes the sin", async (cb) => {
            expect(await sin(23.33)).toBe(Math.sin(23.33));
            cb();
        });
    });

    describe("cos", () => {
        it("computes the cos", async (cb) => {
            expect(await cos(23.33)).toBe(Math.cos(23.33));
            cb();
        });
    });
});
