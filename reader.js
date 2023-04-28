const fs = require("fs");
const fsp = fs.promises;

function getGridPos(index, width) {
    let row = Math.floor(index / width);
    let col = index % width;
    return [row, col];
}

async function readLE() {

}

async function readBE() {
    
}
   

async function getIndexValue(file, index) {
    let fh = await fsp.open(file, "r")
    console.log(fh);
    let buffer = Buffer.alloc(10);
    await fh.read(buffer, 0, 10);
    let byteOrder = buffer.toString("utf-8", 0, 2);
    byteOrder = byteOrder == "II" ? "little" : "big";
    //note bind is slow, maybe just have separate functions for each
    // int32Reader = byteOrder == "little" ? buffer.readInt32LE.bind(buffer) : buffer.readInt32BE.bind(buffer);
    // int16Reader = byteOrder == "little" ? buffer.readInt16LE.bind(buffer) : buffer.readInt16BE.bind(buffer);
    //validate file
    let code = buffer.readInt16LE(2);
    if(code !== 42) {
        throw new Error("Not a valid tiff file.");
    }
    let ifdOffset = buffer.readInt32LE(4);
    await fh.read(buffer, 0, 2, ifdOffset);
    let entries = buffer.readInt16LE();

    let tagsToFind = 2;
    let i = 0;
    // while() {
    //     if()
    //     i++;
    // }

    await fh.read(buffer, 0, 2);
    let tag = buffer.readInt16BE();
    console.log(entries, tag)
    
    
    // buffer.readInt32LE();
    // fh.read<Buffer>(buffer, 8, 2, 8);
    fh.close();
}

async function timing() {
    console.time("test");
    await getIndexValue("rainfall_new_month_statewide_data_map_2022_01.tif", 0);
    console.timeEnd("test");
}

timing();