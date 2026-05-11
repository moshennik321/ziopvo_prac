import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.Signature;
import java.util.ArrayList;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class GenerateDefaultAvDb {
    private static final String DATA_MAGIC = "DB-KLIMOV";
    private static final String MANIFEST_MAGIC = "MF-KLIMOV";

    private record RecordDef(
            UUID id,
            String threatName,
            byte[] firstBytes,
            byte[] remainderHash,
            long remainderLength,
            String fileType,
            long offsetStart,
            long offsetEnd,
            long updatedAtEpochMillis
    ) {
    }

    private record DataRecord(UUID id, long offset, int length) {
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            throw new IllegalArgumentException("Usage: GenerateDefaultAvDb <signing.jks>");
        }

        List<RecordDef> records = new ArrayList<>();
        records.add(new RecordDef(
                UUID.fromString("11111111-1111-1111-1111-111111111111"),
                "Test.PE.Traffic",
                new byte[]{'M', 'Z', (byte) 0x90, 0x00, 0x03, 0x00, 0x00, 0x00},
                hex("5af8f4b1d1637466cc6324c02ac53e2c3b58d4c6974c6a5ee6d14f805a553155"),
                8L,
                "PE",
                0L,
                0L,
                1712419200000L
        ));
        records.add(new RecordDef(
                UUID.fromString("22222222-2222-2222-2222-222222222222"),
                "Test.PS.Mimikatz",
                "Invoke-M".getBytes(StandardCharsets.US_ASCII),
                hex("ecbeb034f4ca297499b4eabc5c2d437c1edbb366a144943aa7488226b859eab0"),
                7L,
                "POWERSHELL",
                0L,
                4096L,
                1712419200000L
        ));

        KeyStore keyStore = KeyStore.getInstance("JKS");
        try (var in = new java.io.FileInputStream(args[0])) {
            keyStore.load(in, "changeit".toCharArray());
        }
        PrivateKey privateKey = (PrivateKey) keyStore.getKey("app-signing", "changeit".toCharArray());

        ByteArrayOutputStream dataBytes = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(dataBytes);
        writeAscii(dataOut, DATA_MAGIC);
        dataOut.writeShort(1);
        dataOut.writeInt(records.size());

        List<DataRecord> dataRecords = new ArrayList<>();
        List<byte[]> recordSignatures = new ArrayList<>();
        long currentOffset = 0L;
        for (RecordDef record : records) {
            byte[] bytes = serializeRecord(record);
            byte[] recordSignature = signRecord(privateKey, record);
            dataRecords.add(new DataRecord(record.id(), currentOffset, bytes.length));
            recordSignatures.add(recordSignature);
            dataOut.write(bytes);
            currentOffset += bytes.length;
        }
        dataOut.flush();

        byte[] dataPayload = dataBytes.toByteArray();
        byte[] dataSha256 = MessageDigest.getInstance("SHA-256").digest(dataPayload);

        ByteArrayOutputStream unsignedManifestBytes = new ByteArrayOutputStream();
        DataOutputStream manifestOut = new DataOutputStream(unsignedManifestBytes);
        writeAscii(manifestOut, MANIFEST_MAGIC);
        manifestOut.writeShort(1);
        manifestOut.writeByte(1);
        manifestOut.writeLong(1712419200000L);
        manifestOut.writeLong(-1L);
        manifestOut.writeInt(records.size());
        manifestOut.write(dataSha256);

        for (int i = 0; i < records.size(); ++i) {
            RecordDef record = records.get(i);
            DataRecord dataRecord = dataRecords.get(i);
            manifestOut.writeLong(record.id().getMostSignificantBits());
            manifestOut.writeLong(record.id().getLeastSignificantBits());
            manifestOut.writeByte(1);
            manifestOut.writeLong(record.updatedAtEpochMillis());
            manifestOut.writeLong(dataRecord.offset());
            manifestOut.writeInt(dataRecord.length());
            writeLengthPrefixedBytes(manifestOut, recordSignatures.get(i));
        }
        manifestOut.flush();
        byte[] unsignedManifest = unsignedManifestBytes.toByteArray();

        Signature signer = Signature.getInstance("SHA256withRSA");
        signer.initSign(privateKey);
        signer.update(unsignedManifest);
        byte[] manifestSignature = signer.sign();

        ByteArrayOutputStream manifestBytes = new ByteArrayOutputStream();
        DataOutputStream signedManifestOut = new DataOutputStream(manifestBytes);
        signedManifestOut.write(unsignedManifest);
        writeLengthPrefixedBytes(signedManifestOut, manifestSignature);
        signedManifestOut.flush();

        System.out.println("DATA_BASE64=" + Base64.getEncoder().encodeToString(dataPayload));
        System.out.println("MANIFEST_BASE64=" + Base64.getEncoder().encodeToString(manifestBytes.toByteArray()));
    }

    private static byte[] serializeRecord(RecordDef record) throws Exception {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(output);
        writeUtf8(out, record.threatName());
        writeLengthPrefixedBytes(out, record.firstBytes());
        writeLengthPrefixedBytes(out, record.remainderHash());
        out.writeLong(record.remainderLength());
        writeUtf8(out, record.fileType());
        out.writeLong(record.offsetStart());
        out.writeLong(record.offsetEnd());
        out.flush();
        return output.toByteArray();
    }

    private static byte[] signRecord(PrivateKey privateKey, RecordDef record) throws Exception {
        Signature signer = Signature.getInstance("SHA256withRSA");
        signer.initSign(privateKey);
        signer.update(buildCanonicalRecordJson(record).getBytes(StandardCharsets.UTF_8));
        return signer.sign();
    }

    private static String buildCanonicalRecordJson(RecordDef record) {
        Map<String, Object> fields = new LinkedHashMap<>();
        fields.put("fileType", record.fileType());
        fields.put("firstBytesHex", toHex(record.firstBytes()));
        fields.put("offsetEnd", record.offsetEnd());
        fields.put("offsetStart", record.offsetStart());
        fields.put("remainderHashHex", toHex(record.remainderHash()));
        fields.put("remainderLength", record.remainderLength());
        fields.put("status", "ACTUAL");
        fields.put("threatName", record.threatName());

        StringBuilder json = new StringBuilder("{");
        boolean first = true;
        for (Map.Entry<String, Object> entry : fields.entrySet()) {
            if (!first) {
                json.append(',');
            }
            first = false;
            json.append('"').append(entry.getKey()).append('"').append(':');
            Object value = entry.getValue();
            if (value instanceof String stringValue) {
                json.append('"').append(escapeJson(stringValue)).append('"');
            } else {
                json.append(value);
            }
        }
        json.append('}');
        return json.toString();
    }

    private static String escapeJson(String value) {
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private static String toHex(byte[] bytes) {
        StringBuilder builder = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            builder.append(String.format("%02x", value));
        }
        return builder.toString();
    }

    private static void writeAscii(DataOutputStream out, String text) throws Exception {
        out.write(text.getBytes(StandardCharsets.US_ASCII));
    }

    private static void writeUtf8(DataOutputStream out, String text) throws Exception {
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        out.writeInt(bytes.length);
        out.write(bytes);
    }

    private static void writeLengthPrefixedBytes(DataOutputStream out, byte[] bytes) throws Exception {
        out.writeInt(bytes.length);
        out.write(bytes);
    }

    private static byte[] hex(String value) {
        int len = value.length();
        byte[] out = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            out[i / 2] = (byte) Integer.parseInt(value.substring(i, i + 2), 16);
        }
        return out;
    }
}
