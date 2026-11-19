import java.awt.AlphaComposite;
import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;
import javax.imageio.ImageIO;

// Invoked by package-project.sh using the JDK already required for Android packaging.
class GenerateLauncherResources {
  private static final String[] DENSITIES = {"mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi"};
  private static final int[] LEGACY_SIZES = {48, 72, 96, 144, 192};
  private static final int[] ADAPTIVE_SIZES = {108, 162, 216, 324, 432};

  private static BufferedImage readAsset(Path assets, String name) throws IOException {
    if (name.isEmpty() || name.startsWith("/") || name.indexOf('\\') >= 0) {
      throw new IOException("Android icon path must be relative to the asset directory: " + name);
    }
    String suffix = name.toLowerCase(Locale.ROOT);
    if (!(suffix.endsWith(".png") || suffix.endsWith(".jpg") || suffix.endsWith(".jpeg"))) {
      throw new IOException("Unsupported Android icon format (use PNG or JPEG): " + name);
    }
    Path path = assets.resolve(name).normalize();
    if (!path.startsWith(assets) || !Files.isRegularFile(path) ||
        !path.toRealPath().startsWith(assets.toRealPath())) {
      throw new IOException("Android icon must be a file inside the asset directory: " + name);
    }
    byte[] header;
    try (var input = Files.newInputStream(path)) {
      header = input.readNBytes(8);
    }
    boolean png = header.length == 8 && (header[0] & 0xff) == 0x89 &&
        header[1] == 'P' && header[2] == 'N' && header[3] == 'G' &&
        header[4] == 13 && header[5] == 10 && header[6] == 26 && header[7] == 10;
    boolean jpeg = header.length >= 3 && (header[0] & 0xff) == 0xff &&
        (header[1] & 0xff) == 0xd8 && (header[2] & 0xff) == 0xff;
    if ((suffix.endsWith(".png") && !png) || (!suffix.endsWith(".png") && !jpeg)) {
      throw new IOException("Android icon contents do not match PNG or JPEG extension: " + name);
    }
    BufferedImage image = ImageIO.read(path.toFile());
    if (image == null || image.getWidth() < 1 || image.getHeight() < 1) {
      throw new IOException("Cannot decode Android icon image: " + name);
    }
    return image;
  }

  private static BufferedImage defaultIcon() {
    BufferedImage image = new BufferedImage(432, 432, BufferedImage.TYPE_INT_ARGB);
    Graphics2D g = image.createGraphics();
    g.setColor(new Color(32, 42, 68));
    g.fillRect(0, 0, 432, 432);
    g.setColor(Color.WHITE);
    g.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 280));
    g.drawString("L", 130, 320);
    g.dispose();
    return image;
  }

  private static BufferedImage scale(BufferedImage source, int size, boolean round) {
    BufferedImage image = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
    Graphics2D g = image.createGraphics();
    g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
    g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
    if (round) {
      g.fillOval(0, 0, size, size);
      g.setComposite(AlphaComposite.SrcIn);
    }
    double factor = Math.min((double) size / source.getWidth(), (double) size / source.getHeight());
    int width = Math.max(1, (int) Math.round(source.getWidth() * factor));
    int height = Math.max(1, (int) Math.round(source.getHeight() * factor));
    g.drawImage(source, (size - width) / 2, (size - height) / 2, width, height, null);
    g.dispose();
    return image;
  }

  private static void writeImages(Path resources, BufferedImage source, String name,
                                  int[] sizes, boolean round) throws IOException {
    for (int i = 0; i < DENSITIES.length; ++i) {
      Path directory = resources.resolve("mipmap-" + DENSITIES[i]);
      Files.createDirectories(directory);
      if (!ImageIO.write(scale(source, sizes[i], round), "png", directory.resolve(name + ".png").toFile())) {
        throw new IOException("Cannot encode launcher icon " + name);
      }
    }
  }

  private static String xml(String value) {
    return value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        .replace("\"", "&quot;").replace("'", "&apos;");
  }

  private static void write(Path path, String content) throws IOException {
    Files.createDirectories(path.getParent());
    Files.writeString(path, content, StandardCharsets.UTF_8);
  }

  public static void main(String[] args) throws Exception {
    if (args.length != 12) throw new IllegalArgumentException("Expected 12 Android packaging arguments");
    Path assets = Path.of(args[0]).toRealPath();
    Path resources = Path.of(args[1]);
    Path manifest = Path.of(args[2]);
    String target = args[3], packageName = args[4], orientation = args[5];
    String appName = args[6].isEmpty() ? target : args[6];
    String versionName = args[7], versionCode = args[8];
    String iconPath = args[9], foregroundPath = args[10];
    String background = args[11];

    BufferedImage legacy = iconPath.isEmpty() ? defaultIcon() : readAsset(assets, iconPath);
    BufferedImage foreground = foregroundPath.isEmpty() ? legacy : readAsset(assets, foregroundPath);
    writeImages(resources, legacy, "ic_launcher", LEGACY_SIZES, false);
    writeImages(resources, legacy, "ic_launcher_round", LEGACY_SIZES, true);
    writeImages(resources, foreground, "ic_launcher_foreground", ADAPTIVE_SIZES, false);
    String backgroundResource;
    if (background.startsWith("#")) {
      if (!background.matches("#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?")) {
        throw new IOException("Invalid adaptive icon background color: " + background);
      }
      write(resources.resolve("values/launcher_colors.xml"), "<resources><color name=\"launcher_background\">" + background + "</color></resources>\n");
      backgroundResource = "@color/launcher_background";
    } else {
      writeImages(resources, readAsset(assets, background), "ic_launcher_background", ADAPTIVE_SIZES, false);
      backgroundResource = "@mipmap/ic_launcher_background";
    }
    String adaptive = "<adaptive-icon xmlns:android=\"http://schemas.android.com/apk/res/android\">" +
        "<background android:drawable=\"" + backgroundResource + "\"/>" +
        "<foreground android:drawable=\"@mipmap/ic_launcher_foreground\"/>" +
        "</adaptive-icon>\n";
    write(resources.resolve("mipmap-anydpi-v26/ic_launcher.xml"), adaptive);
    write(resources.resolve("mipmap-anydpi-v26/ic_launcher_round.xml"), adaptive);
    write(resources.resolve("values/strings.xml"), "<resources><string name=\"app_name\">" + xml(appName) + "</string></resources>\n");
    write(manifest, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
        "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\" package=\"" + xml(packageName) +
        "\" android:versionName=\"" + xml(versionName) + "\" android:versionCode=\"" + versionCode + "\">\n" +
        "<uses-feature android:glEsVersion=\"0x00020000\" android:required=\"true\"/>\n" +
        "<application android:label=\"@string/app_name\" android:icon=\"@mipmap/ic_launcher\" " +
        "android:roundIcon=\"@mipmap/ic_launcher_round\" android:hasCode=\"false\">\n" +
        "<activity android:name=\"android.app.NativeActivity\" android:launchMode=\"singleTask\" " +
        "android:screenOrientation=\"" + xml(orientation) + "\" " +
        "android:configChanges=\"orientation|keyboardHidden|screenSize\" android:exported=\"true\">\n" +
        "<meta-data android:name=\"android.app.lib_name\" android:value=\"main\"/>\n" +
        "<intent-filter><action android:name=\"android.intent.action.MAIN\"/>" +
        "<category android:name=\"android.intent.category.LAUNCHER\"/></intent-filter>\n" +
        "</activity></application></manifest>\n");
  }
}
