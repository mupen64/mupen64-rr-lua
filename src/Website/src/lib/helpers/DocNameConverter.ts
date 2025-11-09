export function doc_filesystem_to_friendly_name(name: string) {
    name = name.split("-").join(" ");
    name = name.charAt(0).toUpperCase() + name.slice(1);
    return name;
}