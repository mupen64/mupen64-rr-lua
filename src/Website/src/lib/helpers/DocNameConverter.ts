export function doc_name_to_friendly_name(name: string): string {
    // Remove optional order prefix like "1. "
    name = name.replace(/^\d+\.\s*/, '');
    // Replace dashes with spaces
    name = name.split('-').join(' ');
    // Capitalize first letter
    name = name.charAt(0).toUpperCase() + name.slice(1);
    return name;
}
